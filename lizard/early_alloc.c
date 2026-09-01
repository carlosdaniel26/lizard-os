#include <lizard/early_alloc.h>

#include <lizard/boot.h>
#include <lizard/debug.h>
#include <lizard/helpers.h>
#include <lizard/panic.h>
#include <lizard/pgtable.h>

#include <lizard/init.h>
#include <nolibc/stdio.h>
#include <nolibc/types.h>

u64 highest_addr = 0;
u64 hhdm_offset = 0;

vaddr_t early_base;
vaddr_t early_end;
vaddr_t early_current;

static int early_alloc_init()
{
    struct boot_info *bi = boot_info_ptr;
    if (!bi || bi->magic != BOOT_INFO_MAGIC || !bi->mmap_count)
        kpanic("NO MEMORY MAP FROM LOADER");

    paddr_t largest_base = 0;
    size_t largest_size = 0;

    for (u64 i = 0; i < bi->mmap_count; i++)
    {
        struct bi_mmap_entry *entry = &bi->mmap[i];

        if (entry->type == BI_USABLE)
        {
            if (entry->len > largest_size)
            {
                largest_base = entry->base;
                largest_size = entry->len;
            }

            if (entry->base + entry->len > highest_addr)
            {
                highest_addr = entry->base + entry->len;
            }
        }
    }

    early_base = align_up(largest_base, PAGE_SIZE);
    early_end = align_down(largest_base + largest_size, PAGE_SIZE);
    early_current = early_base;

    hhdm_offset = bi->hhdm_base;

    return 0;
}

early_initcall(early_alloc_init);

vaddr_t early_alloc(size_t size, size_t align)
{
    if (align == 0) align = sizeof(void *);

    vaddr_t curr = align_up(early_current, align);
    vaddr_t next = curr + size;

    if (next > early_end) kpanic("OUT OF MEMORY IN EARLY ALLOCATOR");

    early_current = next;
    return curr + hhdm_offset;
}