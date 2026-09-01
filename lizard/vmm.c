#include <lizard/boot.h>
#include <lizard/buddy.h>
#include <lizard/debug.h>
#include <lizard/early_alloc.h>
#include <lizard/framebuffer.h>
#include <lizard/helpers.h>
#include <lizard/kernelcfg.h>
#include <lizard/memory_map.h>
#include <lizard/panic.h>
#include <lizard/pgtable.h>

#include <lizard/init.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <nolibc/types.h>
#include <lizard/vmm.h>

#define DIV_UP(a, b) (((a) + (b) - 1) / (b))

extern u64 kernel_stack[];
extern u32 kernel_start;
extern u32 kernel_end;

vaddr_t current_pml4 = 0;
vaddr_t kernel_pml4 = 0;
extern u64 hhdm_offset;

static int vmm_init(void)
{
    struct boot_info *bi = boot_info_ptr;
    if (!bi) kpanic("VMM: No boot_info");

    kernel_pml4 = pgtable_alloc_table();
    current_pml4 = kernel_pml4;

    /* 1. Map the kernel */
    vaddr_t vstart = align_down((u64)&kernel_start, PAGE_SIZE);
    vaddr_t vend = align_up((u64)&kernel_end, PAGE_SIZE);
    paddr_t phys = bi->kernel_phys_base;
    paddr_t pstart = phys - ((u64)&kernel_start - vstart);
    u64 kernel_pages = (vend - vstart) / PAGE_SIZE;

    pgtable_maprange(kernel_pml4, vstart, pstart, kernel_pages, PAGE_PRESENT | PAGE_WRITABLE);

    /* 2. Map the framebuffer (if the loader gave us one) */
    if (framebuffer_length)
        pgtable_maprange(kernel_pml4, (vaddr_t)framebuffer, (paddr_t)framebuffer - hhdm_offset,
                         framebuffer_length / PAGE_SIZE, PAGE_PRESENT | PAGE_WRITABLE);

    /* 3. Map every region in the memory map to HHDM */
    for (u64 i = 0; i < bi->mmap_count; i++)
    {
        struct bi_mmap_entry *entry = &bi->mmap[i];

        /* Align to page boundaries */
        paddr_t start = align_down(entry->base, PAGE_SIZE);
        paddr_t end = align_up(entry->base + entry->len, PAGE_SIZE);

        u64 pages = (end - start) / PAGE_SIZE;
        pgtable_maprange(kernel_pml4, (vaddr_t)start + hhdm_offset, start, pages, PAGE_PRESENT | PAGE_WRITABLE);
    }

    pgtable_switch(kernel_pml4);

    /* Framebuffer is reachable now - wipe the firmware's leftover text. */
    if (framebuffer_length)
        clear_framebuffer();

    return 0;
}

postcore_initcall(vmm_init);

void vmm_map_page(vaddr_t vaddr, paddr_t paddr, u64 flags)
{
    pgtable_map(current_pml4, vaddr, paddr, flags);
}

void vmm_unmap_page(vaddr_t vaddr)
{
    pgtable_unmap(current_pml4, vaddr);
}

void *vmm_alloc(vaddr_t pml4, vaddr_t vaddr, u64 flags)
{
    void *ptr = (void *)buddy_alloc(0);
    pgtable_map(pml4, vaddr, (paddr_t)ptr - hhdm_offset, flags);
    return ptr;
}

void *vmm_alloc_page(void)
{
    vaddr_t ptr = (vaddr_t)buddy_alloc(0);
    pgtable_map(current_pml4, ptr, (paddr_t)ptr - hhdm_offset, PAGE_PRESENT | PAGE_WRITABLE);
    return (void *)ptr;
}

void vmm_free_page(vaddr_t vaddr)
{
    pgtable_unmap(current_pml4, vaddr);
}

void vmm_switch_pml4(vaddr_t pml4)
{
    current_pml4 = pml4;
    pgtable_switch(pml4);
}