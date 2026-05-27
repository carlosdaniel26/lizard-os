#include <lizard/early_alloc.h>

#include <lizard/debug.h>
#include <lizard/helpers.h>
#include <lizard/limine.h>
#include <lizard/panic.h>
#include <lizard/pgtable.h>

#include <lizard/init.h>
#include <nolibc/stdio.h>
#include <nolibc/types.h>

__attribute__((used, section(".limine_requests"))) volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0};

u64 highest_addr = 0;
u64 hhdm_offset = 0;

vaddr_t early_base;
vaddr_t early_end;
vaddr_t early_current;

static int early_alloc_init()
{
    struct limine_memmap_response *response = memmap_request.response;
    if (!response || !response->entry_count) kpanic("NO MEMORY MAP FROM LIMINE");

    paddr_t largest_base = 0;
    size_t largest_size = 0;

    for (size_t i = 0; i < response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            if (entry->length > largest_size)
            {
                largest_base = entry->base;
                largest_size = entry->length;
            }

            if (entry->base + entry->length > highest_addr)
            {
                highest_addr = entry->base + entry->length;
            }
        }
    }

    early_base = align_up(largest_base, PAGE_SIZE);
    early_end = align_down(largest_base + largest_size, PAGE_SIZE);
    early_current = early_base;

    hhdm_offset = (u64)hhdm_request.response->offset;

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