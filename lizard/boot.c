#include <lizard/boot.h>
#include <lizard/panic.h>
#include <nolibc/types.h>

/* Published by head.S (_start_high) just before it calls kmain(). */
struct boot_info *boot_info_ptr = NULL;

#define BI_MMAP_MAX 256

static struct boot_info bi_copy;
static struct bi_mmap_entry bi_mmap_copy[BI_MMAP_MAX];

void boot_info_relocate(void)
{
    struct boot_info *src = boot_info_ptr;

    if (!src || src->magic != BOOT_INFO_MAGIC)
        kpanic("boot_info: bad or missing hand-off from loader");

    bi_copy = *src;

    u64 n = src->mmap_count;
    if (n > BI_MMAP_MAX) n = BI_MMAP_MAX;
    for (u64 i = 0; i < n; i++)
        bi_mmap_copy[i] = src->mmap[i];

    bi_copy.mmap_count = n;
    bi_copy.mmap = bi_mmap_copy;

    boot_info_ptr = &bi_copy;
}

/* head.S reads these by hard-coded offset - keep the two in sync. */
_Static_assert(offsetof(struct boot_info, hhdm_base) == 0x08, "BI_HHDM");
_Static_assert(offsetof(struct boot_info, highest_addr) == 0x10, "BI_HIGHEST");
_Static_assert(offsetof(struct boot_info, kernel_phys_base) == 0x18, "BI_KPHYS");
_Static_assert(offsetof(struct boot_info, kernel_virt_base) == 0x20, "BI_KVIRT");
_Static_assert(offsetof(struct boot_info, kernel_size) == 0x28, "BI_KSIZE");
_Static_assert(offsetof(struct boot_info, scratch_phys_base) == 0x30, "BI_SCRATCH");
_Static_assert(offsetof(struct boot_info, scratch_size) == 0x38, "BI_SCRATCH_SZ");
