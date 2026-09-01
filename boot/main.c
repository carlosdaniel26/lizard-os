/*
 * boot/main.c - lizard's UEFI loader.
 *
 * Loads /kernel.elf from the same volume, copies its PT_LOAD segments into one
 * contiguous, 2 MiB-aligned physical region, fills a struct boot_info, exits
 * boot services and jumps to the kernel ELF entry point (lizard/head.S:_start)
 * with the boot_info pointer in RDI (SysV).
 *
 * The kernel builds its own page tables and performs the higher-half jump; this
 * loader does no paging.
 */

#include "efi.h"
#include "../lizard/boot.h"

#define KERNEL_PATH  L"kernel.elf"
#define KERNEL_CMDLINE "root=ata0p0"

#define SCRATCH_PAGES 32           /* 128 KiB for head.S page tables      */
#define KALIGN        0x200000ULL  /* head.S maps the kernel with 2 MiB pages */

/* --- minimal ELF64 --- */
typedef struct {
    u8  e_ident[16];
    u16 e_type, e_machine;
    u32 e_version;
    u64 e_entry, e_phoff, e_shoff;
    u32 e_flags;
    u16 e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum, e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    u32 p_type, p_flags;
    u64 p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align;
} Elf64_Phdr;

#define PT_LOAD 1

static EFI_SYSTEM_TABLE  *ST;
static EFI_BOOT_SERVICES  *BS;

/* ------------------------------------------------------------------ helpers */

static void print(const CHAR16 *s)
{
    ST->ConOut->OutputString(ST->ConOut, (CHAR16 *)s);
}

static void print_hex(u64 v)
{
    CHAR16 buf[19];
    int i;
    buf[0] = L'0'; buf[1] = L'x';
    for (i = 0; i < 16; i++)
        buf[2 + i] = L"0123456789ABCDEF"[(v >> ((15 - i) * 4)) & 0xF];
    buf[18] = 0;
    print(buf);
}

static void die(const CHAR16 *msg, EFI_STATUS st)
{
    print(L"loader: "); print(msg); print(L" status="); print_hex(st); print(L"\r\n");
    for (;;)
        __asm__ __volatile__("hlt");
}

static void *mem_set(void *d, int c, u64 n)
{
    u8 *p = d;
    while (n--) *p++ = (u8)c;
    return d;
}

static void *mem_cpy(void *d, const void *s, u64 n)
{
    u8 *pd = d;
    const u8 *ps = s;
    while (n--) *pd++ = *ps++;
    return d;
}

static void str_copy(char *d, const char *s, u64 max)
{
    u64 i = 0;
    for (; s[i] && i + 1 < max; i++) d[i] = s[i];
    d[i] = 0;
}

/* --- COM1, for breadcrumbs after ExitBootServices --- */
static inline void outb(u16 port, u8 val)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline u8 inb(u16 port)
{
    u8 r;
    __asm__ __volatile__("inb %1, %0" : "=a"(r) : "Nd"(port));
    return r;
}
static void com1_init(void)
{
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x01);   /* 115200 */
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);   /* 8N1 */
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}
static void com1_puts(const char *s)
{
    for (; *s; s++) {
        int spin = 100000;
        while (spin-- && !(inb(0x3F8 + 5) & 0x20))
            ;
        outb(0x3F8, (u8)*s);
    }
}

/* ---------------------------------------------------------------- read file */

static u8 *read_kernel(EFI_HANDLE image, UINTN *out_size)
{
    EFI_GUID li_guid  = EFI_LOADED_IMAGE_PROTOCOL_GUID;
    EFI_GUID fs_guid  = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
    EFI_GUID fi_guid  = EFI_FILE_INFO_GUID;
    EFI_STATUS st;

    EFI_LOADED_IMAGE_PROTOCOL *li;
    st = BS->HandleProtocol(image, &li_guid, (void **)&li);
    if (EFI_ERROR(st)) die(L"HandleProtocol(loaded image)", st);

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *fs;
    st = BS->HandleProtocol(li->DeviceHandle, &fs_guid, (void **)&fs);
    if (EFI_ERROR(st)) die(L"HandleProtocol(simple fs)", st);

    EFI_FILE_PROTOCOL *root;
    st = fs->OpenVolume(fs, &root);
    if (EFI_ERROR(st)) die(L"OpenVolume", st);

    EFI_FILE_PROTOCOL *kf;
    st = root->Open(root, &kf, (CHAR16 *)KERNEL_PATH, EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(st)) die(L"Open(\\kernel.elf)", st);

    u8 info_buf[sizeof(EFI_FILE_INFO) + 32];
    UINTN info_size = sizeof(info_buf);
    st = kf->GetInfo(kf, &fi_guid, &info_size, info_buf);
    if (EFI_ERROR(st)) die(L"GetInfo", st);
    u64 fsize = ((EFI_FILE_INFO *)info_buf)->FileSize;

    void *buf;
    st = BS->AllocatePool(EfiLoaderData, fsize, &buf);
    if (EFI_ERROR(st)) die(L"AllocatePool(kernel)", st);

    UINTN rd = fsize;
    st = kf->Read(kf, &rd, buf);
    if (EFI_ERROR(st) || rd != fsize) die(L"Read(kernel)", st);
    kf->Close(kf);

    *out_size = fsize;
    return buf;
}

/* ------------------------------------------------------------ load segments */

static void load_kernel(u8 *elf, struct boot_info *bi, u64 *entry_off)
{
    Elf64_Ehdr *eh = (Elf64_Ehdr *)elf;
    if (eh->e_ident[0] != 0x7F || eh->e_ident[1] != 'E' ||
        eh->e_ident[2] != 'L'  || eh->e_ident[3] != 'F')
        die(L"kernel.elf: bad magic", 0);

    u64 vmin = ~0ULL, vmax = 0;
    for (u16 i = 0; i < eh->e_phnum; i++) {
        Elf64_Phdr *ph = (Elf64_Phdr *)(elf + eh->e_phoff + (u64)i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD || ph->p_memsz == 0) continue;
        if (ph->p_vaddr < vmin) vmin = ph->p_vaddr;
        if (ph->p_vaddr + ph->p_memsz > vmax) vmax = ph->p_vaddr + ph->p_memsz;
    }
    if (vmin == ~0ULL) die(L"kernel.elf: no PT_LOAD", 0);

    u64 span = (vmax - vmin + EFI_PAGE_SIZE - 1) & ~(u64)(EFI_PAGE_SIZE - 1);
    /* over-allocate so we can 2 MiB-align the base head.S maps from */
    UINTN pages = (span + KALIGN) / EFI_PAGE_SIZE;

    EFI_PHYSICAL_ADDRESS raw = 0;
    EFI_STATUS st = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &raw);
    if (EFI_ERROR(st)) die(L"AllocatePages(kernel)", st);

    u64 base = (raw + (KALIGN - 1)) & ~(KALIGN - 1);
    mem_set((void *)base, 0, span);

    for (u16 i = 0; i < eh->e_phnum; i++) {
        Elf64_Phdr *ph = (Elf64_Phdr *)(elf + eh->e_phoff + (u64)i * eh->e_phentsize);
        if (ph->p_type != PT_LOAD || ph->p_memsz == 0) continue;
        u8 *dst = (u8 *)(base + (ph->p_vaddr - vmin));
        mem_cpy(dst, elf + ph->p_offset, ph->p_filesz);
        /* [filesz, memsz) already zeroed by mem_set above */
    }

    bi->kernel_phys_base = base;
    bi->kernel_virt_base = vmin;
    bi->kernel_size      = span;
    *entry_off           = eh->e_entry - vmin;
}

/* --------------------------------------------------------------- mmap xlate */

static u32 xlate_type(u32 efi)
{
    switch (efi) {
    case EfiConventionalMemory:
    case EfiBootServicesCode:
    case EfiBootServicesData:
        return BI_USABLE;
    case EfiLoaderCode:
    case EfiLoaderData:
        return BI_LOADER;
    case EfiACPIReclaimMemory:
        return BI_ACPI_RECLAIM;
    case EfiACPIMemoryNVS:
        return BI_ACPI_NVS;
    case EfiUnusableMemory:
        return BI_BAD;
    default:
        return BI_RESERVED;
    }
}

/* --------------------------------------------------------------- graphics */

static void query_gop(struct boot_info *bi)
{
    EFI_GUID gop_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = 0;

    if (EFI_ERROR(BS->LocateProtocol(&gop_guid, 0, (void **)&gop)) || !gop || !gop->Mode) {
        print(L"no GOP; serial-only\r\n");
        return;
    }

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *mi = gop->Mode->Info;

    bi->fb_base   = gop->Mode->FrameBufferBase;
    bi->fb_size   = gop->Mode->FrameBufferSize;
    bi->fb_width  = mi->HorizontalResolution;
    bi->fb_height = mi->VerticalResolution;
    bi->fb_pitch  = mi->PixelsPerScanLine * 4;
    bi->fb_bpp    = 32;
    bi->fb_format = (mi->PixelFormat == PixelRedGreenBlueReserved8BitPerColor)
                        ? BI_FB_RGBX : BI_FB_BGRX;

    print(L"GOP "); print_hex(bi->fb_width); print(L" x "); print_hex(bi->fb_height);
    print(L" @ "); print_hex(bi->fb_base); print(L" pitch "); print_hex(bi->fb_pitch);
    print(L"\r\n");
}

/* ---------------------------------------------------------------- efi_main */

EFIAPI EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *st)
{
    ST = st;
    BS = st->BootServices;

    ST->ConOut->ClearScreen(ST->ConOut);
    print(L"lizard loader\r\n");

    UINTN elf_size;
    u8 *elf = read_kernel(image, &elf_size);
    print(L"kernel.elf loaded, "); print_hex(elf_size); print(L" bytes\r\n");

    /* one 8-page block holds boot_info + the mmap array */
    EFI_PHYSICAL_ADDRESS bi_pages = 0;
    if (EFI_ERROR(BS->AllocatePages(AllocateAnyPages, EfiLoaderData, 8, &bi_pages)))
        die(L"AllocatePages(boot_info)", 0);
    struct boot_info *bi = (struct boot_info *)bi_pages;
    struct bi_mmap_entry *bi_mmap = (struct bi_mmap_entry *)(bi_pages + EFI_PAGE_SIZE);
    const u64 bi_mmap_cap = (7 * EFI_PAGE_SIZE) / sizeof(struct bi_mmap_entry);
    mem_set(bi, 0, sizeof(*bi));

    u64 entry_off = 0;
    load_kernel(elf, bi, &entry_off);
    print(L"kernel phys="); print_hex(bi->kernel_phys_base);
    print(L" size=");       print_hex(bi->kernel_size);
    print(L" entry_off=");  print_hex(entry_off); print(L"\r\n");

    EFI_PHYSICAL_ADDRESS scratch = 0;
    if (EFI_ERROR(BS->AllocatePages(AllocateAnyPages, EfiLoaderData, SCRATCH_PAGES, &scratch)))
        die(L"AllocatePages(scratch)", 0);
    mem_set((void *)scratch, 0, SCRATCH_PAGES * EFI_PAGE_SIZE);
    bi->scratch_phys_base = scratch;
    bi->scratch_size      = (u64)SCRATCH_PAGES * EFI_PAGE_SIZE;

    bi->magic     = BOOT_INFO_MAGIC;
    bi->hhdm_base = BOOT_HHDM_BASE;
    bi->mmap      = bi_mmap;
    str_copy(bi->cmdline, KERNEL_CMDLINE, sizeof(bi->cmdline));

    query_gop(bi);

    com1_init();
    print(L"exiting boot services...\r\n");

    /* GetMemoryMap -> build bi->mmap -> ExitBootServices, retrying on a stale key */
    for (int attempt = 0; attempt < 8; attempt++) {
        UINTN map_size = 0, map_key = 0, desc_size = 0;
        u32 desc_ver = 0;
        EFI_MEMORY_DESCRIPTOR *map = 0;

        EFI_STATUS s = BS->GetMemoryMap(&map_size, 0, &map_key, &desc_size, &desc_ver);
        if (s != EFI_BUFFER_TOO_SMALL) die(L"GetMemoryMap(size)", s);
        map_size += 4 * desc_size;
        if (EFI_ERROR(BS->AllocatePool(EfiLoaderData, map_size, (void **)&map)))
            die(L"AllocatePool(mmap)", 0);
        s = BS->GetMemoryMap(&map_size, map, &map_key, &desc_size, &desc_ver);
        if (EFI_ERROR(s)) { BS->FreePool(map); continue; }

        u64 n = 0, highest = 0;
        for (UINTN off = 0; off < map_size && n < bi_mmap_cap; off += desc_size) {
            EFI_MEMORY_DESCRIPTOR *d = (EFI_MEMORY_DESCRIPTOR *)((u8 *)map + off);
            u64 len = d->NumberOfPages * EFI_PAGE_SIZE;
            u32 type = xlate_type(d->Type);
            /* Never hand the kernel the low 1 MiB (IVT/BDA/EBDA/VGA/option ROMs). */
            if (type == BI_USABLE && d->PhysicalStart < 0x100000)
                type = BI_RESERVED;
            bi_mmap[n].base = d->PhysicalStart;
            bi_mmap[n].len  = len;
            bi_mmap[n].type = type;
            bi_mmap[n]._pad = 0;
            if (type == BI_USABLE && d->PhysicalStart + len > highest)
                highest = d->PhysicalStart + len;
            n++;
        }
        bi->mmap_count   = n;
        bi->highest_addr = highest;

        s = BS->ExitBootServices(image, map_key);
        if (!EFI_ERROR(s))
            break;                     /* success: firmware is ours now */
        BS->FreePool(map);
        if (attempt == 7) die(L"ExitBootServices", s);
    }

    com1_puts("L");                    /* loader: about to enter kernel */

    void (*entry)(struct boot_info *) =
        (void (*)(struct boot_info *))(bi->kernel_phys_base + entry_off);
    entry(bi);

    for (;;)
        __asm__ __volatile__("hlt");
}
