#include <buddy.h>
#include <types.h>
#include <string.h>
#include <stdio.h>
#include <limine.h>
#include <helpers.h>
#include <early_alloc.h>
#include <pgtable.h>
#include <panic.h>

extern u32 kernel_start;
extern u32 kernel_end;

#define IS_ALIGNED(pfn, order) (((pfn) & ((1UL << (order)) - 1)) == 0)
#define BLOCK_FITS(pfn, order, end_pfn) ((pfn) + (1UL << (order)) <= (long unsigned int)(end_pfn))

#define KERNEL_STACK_SIZE 0x4000 /* 16 KiB */
extern u8 kernel_stack[KERNEL_STACK_SIZE];

__attribute__((used, section(".limine_requests"))) volatile struct limine_executable_address_request
    kernel_address_request
    = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 0};


BuddyAllocator buddy;

unsigned int pages_to_order(unsigned int pages)
{
    unsigned int order = 0;
    unsigned int size = 1;

    while (size < pages) {
        size <<= 1;
        order++;
    }
    return order;
}

static size_t detect_page_count(void)
{
    struct limine_memmap_response *resp = memmap_request.response;
    uintptr_t max_addr = 0;

    if (!resp)
        return 0;

    const uint64_t entry_count = resp->entry_count;
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *e = resp->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;
        uintptr_t end = e->base + e->length;
        if (end > max_addr)
            max_addr = end;
    }

    return (size_t)(align_up(max_addr, PAGE_SIZE) / PAGE_SIZE);
}

static inline void buddy_add_block(BuddyPage *page, uint8_t order)
{
    page->flags = PAGE_FREE;
    page->order = order;

    list_add(&page->list,
             &buddy.free_areas[order].free_list);

    buddy.free_areas[order].free_count++;
}

void buddy_init()
{
    buddy.page_count = detect_page_count();
    buddy.pages = early_alloc(
        buddy.page_count * sizeof(BuddyPage), 
        0);

    const size_t page_count = buddy.page_count;
    /* init reserved to later free just the usable parts */
    for (size_t i = 0; i < page_count; i++)
    {
        buddy.pages[i].flags = PAGE_RESERVED;
        buddy.pages[i].order = 0;
    }

	/* initialize free lists */
	for (int order = 0; order <= MAX_ORDER; order++)
	{
        buddy.free_areas[order].free_list.next = &buddy.free_areas[order].free_list;
		buddy.free_areas[order].free_list.prev = &buddy.free_areas[order].free_list;

        buddy.free_areas[order].free_count = 0;
    }

    /* mark usable pages */
    struct limine_memmap_response *resp =
        memmap_request.response;
    
    if (!resp)
        return;

    const u64 entry_count = resp->entry_count;
    for (u64 i = 0; i < entry_count; i++)
    {
        struct limine_memmap_entry *e = resp->entries[i];

        if (e->type != LIMINE_MEMMAP_USABLE)
            continue; /* skip non-usable regions */

        uintptr_t base = align_up(e->base, PAGE_SIZE);
        uintptr_t end = align_down(e->base + e->length, PAGE_SIZE);

        if (base >= end)
            continue; /* unusable region */

        int start_pfn = base / PAGE_SIZE;
        int end_pfn = end / PAGE_SIZE;

        int curr_pfn = start_pfn;
        
        /* allright this is usable */
        while(curr_pfn < end_pfn)
        {
            u8 order = MAX_ORDER;

            /* find biggest block */
            while(order > 0)
            {
                if (! IS_ALIGNED(curr_pfn, order) ||
                    ! BLOCK_FITS(curr_pfn, order, end_pfn))
                {
                    order--; /* try a smaller block*/
                    continue;
                }

                break; /* found largest block */
            }

            /* check if any block is being used */
            int can_use = 1;
            for (u64 p = 0; p < (1UL << order); p++)
            {
                if (buddy.pages[curr_pfn + p].flags == PAGE_FREE)
                {
                    can_use = 0;
                    break;
                }
            }

            if (can_use)
            {
                buddy_add_block(&buddy.pages[curr_pfn], order);

                for (u64 j = 0; j < (1UL << order); j++)
                    buddy.pages[curr_pfn + j].flags = PAGE_FREE;
            }

            curr_pfn += 1UL << order;
        }
    }

    /* reserve early_alloc allocated memory */
    int start_page = align_down(early_base, PAGE_SIZE) / PAGE_SIZE;
    int end_page = align_up(early_current, PAGE_SIZE) / PAGE_SIZE;

    for (int p = start_page; p < end_page; p++)
    {
        buddy.pages[p].flags = PAGE_RESERVED;
    }

    /* reserve kernel memory */
	uintptr_t kernel_size = (uintptr_t)&kernel_end - (uintptr_t)&kernel_start;

	uintptr_t kernel_phys_start = kernel_address_request.response->physical_base;
	uintptr_t kernel_phys_end = kernel_phys_start + kernel_size;

    u64 kernel_start_pfn = kernel_phys_start / PAGE_SIZE;
    u64 kernel_end_pfn   = kernel_phys_end / PAGE_SIZE;

    for (u64 pfn = kernel_start_pfn; pfn < kernel_end_pfn; pfn++)
        buddy.pages[pfn].flags = PAGE_RESERVED;

}

void *buddy_alloc(int order)
{
    if (order > MAX_ORDER)
        return NULL; /* too big */

    const i64 block_size = 1UL << order;

    for (size_t pfn = 0; pfn + block_size <= buddy.page_count; pfn++)
    {
        if ((pfn & (block_size - 1)) != 0)
            continue;

        int can_use = 1;
        for (i64 i = 0; i < block_size; i++)
        {
            if (buddy.pages[pfn + i].flags != PAGE_FREE)
            {
                can_use = 0;
                break;
            }
        }

        if (!can_use)
            continue;

        for (i64 i = 0; i < block_size; i++)
            buddy.pages[pfn + i].flags = PAGE_ALLOCATED;

        uintptr_t addr = (uintptr_t)(pfn * PAGE_SIZE) + hhdm_offset;
        if (addr % PAGE_SIZE != 0)
            kpanic("buddy_alloc: allocated address not aligned");
        return (void *)addr;
    }

    kpanic("buddy_alloc: no block found");
    return NULL; /* no block found */
}

void buddy_free(void *addr, int order)
{
    addr = (void *)((uintptr_t)addr - hhdm_offset);
    if (!addr || order > MAX_ORDER)
        return;

    const int block_size = 1UL << order;
    const size_t pfn = (uintptr_t)addr / PAGE_SIZE;

    if (pfn + block_size > buddy.page_count)
        return;

    for (int i = 0; i < block_size; i++)
    {
        buddy.pages[pfn + i].flags = PAGE_FREE;
        buddy.pages[pfn + i].order = 0;
    }
}