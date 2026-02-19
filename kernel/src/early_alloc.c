#include <early_alloc.h>

#include <debug.h>
#include <helpers.h>
#include <limine.h>
#include <panic.h>
#include <pgtable.h>
#include <stddef.h>
#include <stdio.h>
#include <types.h>

__attribute__((used, section(".limine_requests"))) volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0};

u64 highest_addr = 0;
u64 hhdm_offset = 0;

uintptr_t early_base;
uintptr_t early_end;
uintptr_t early_current;

void early_alloc_init(void)
{
    struct limine_memmap_response *response = memmap_request.response;
    if (!response || !response->entry_count) kpanic("NO MEMORY MAP FROM LIMINE");

    uintptr_t largest_base = 0;
    uintptr_t largest_size = 0;

    for (size_t i = 0; i < response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = response->entries[i];

        debug_printf("MEMMAP ENTRY: base=0x%x length=0x%x type=%u\n", entry->base, entry->length,
                     entry->type);

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
                debug_printf("highest_addr: 0x%x = base 0x%x + length 0x%x\n", highest_addr, entry->base,
                             entry->length);
            }
        }
    }

    early_base = align_up(largest_base, PAGE_SIZE);
    early_end = align_down(largest_base + largest_size, PAGE_SIZE);
    early_current = early_base;

    hhdm_offset = (uintptr_t)hhdm_request.response->offset;
}

void *early_alloc(size_t size, size_t align)
{
    if (align == 0) align = sizeof(void *);

    uintptr_t curr = align_up(early_current, align);
    uintptr_t next = curr + size;

    if (next > early_end) kpanic("OUT OF MEMORY IN EARLY ALLOCATOR");

    early_current = next;
    return (void *)curr + hhdm_offset;
}