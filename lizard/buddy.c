#include <lizard/buddy.h>
#include <lizard/early_alloc.h>
#include <lizard/helpers.h>
#include <lizard/init.h>
#include <lizard/limine.h>
#include <lizard/panic.h>
#include <lizard/pgtable.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <nolibc/types.h>

extern u32 kernel_start;
extern u32 kernel_end;

#define IS_ALIGNED(pfn, order) (((pfn) & ((1UL << (order)) - 1)) == 0)
#define BLOCK_FITS(pfn, order, end_pfn) ((pfn) + (1UL << (order)) <= (long unsigned int)(end_pfn))

#define KERNEL_STACK_SIZE 0x4000 /* 16 KiB */
extern u8 kernel_stack[KERNEL_STACK_SIZE];

__attribute__((used, section(".limine_requests"))) volatile struct limine_executable_address_request
    kernel_address_request = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 0};

struct buddy_allocator buddy;

unsigned int pages_to_order(unsigned int pages)
{
    if (pages <= 1)
        return 0;

    return 32 - __builtin_clz(pages - 1);
}

static size_t detect_page_count(void)
{
    struct limine_memmap_response *resp = memmap_request.response;
    paddr_t max_addr = 0;

    if (!resp) return 0;

    const uint64_t entry_count = resp->entry_count;
    for (uint64_t i = 0; i < entry_count; i++)
    {
        struct limine_memmap_entry *e = resp->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;
        paddr_t end = e->base + e->length;
        if (end > max_addr) max_addr = end;
    }

    return (size_t)(align_up(max_addr, PAGE_SIZE) / PAGE_SIZE);
}

static inline void buddy_add_block(struct buddy_page *page, uint8_t order)
{
    page->flags = PAGE_FREE;
    page->order = order;

    list_add(&page->list, &buddy.free_areas[order].free_list);

    buddy.free_areas[order].free_count++;
}

int buddy_init()
{
    buddy.page_count = detect_page_count();
    buddy.pages = (struct buddy_page *)early_alloc(buddy.page_count * sizeof(struct buddy_page), 0);

    /* initialize free lists */
    for (int order = 0; order <= MAX_ORDER; order++)
    {
        InitListHead(&buddy.free_areas[order].free_list);
        buddy.free_areas[order].free_count = 0;
    }

    /* Mark EVERYTHING as reserved first. This is fast as it's just a loop. */
    memset(buddy.pages, 0, buddy.page_count * sizeof(struct buddy_page));
    for (size_t i = 0; i < buddy.page_count; i++)
    {
        buddy.pages[i].flags = PAGE_RESERVED;
    }

    /* Determine kernel and early_alloc ranges to keep them reserved. */
    paddr_t kernel_phys_start = kernel_address_request.response->physical_base;
    vaddr_t kernel_virt_start = (vaddr_t)&kernel_start;
    vaddr_t kernel_virt_end = (vaddr_t)&kernel_end;
    size_t kernel_size = kernel_virt_end - kernel_virt_start;
    paddr_t kernel_phys_end = kernel_phys_start + kernel_size;

    u64 kernel_start_pfn = kernel_phys_start / PAGE_SIZE;
    u64 kernel_end_pfn = align_up(kernel_phys_end, PAGE_SIZE) / PAGE_SIZE;

    u64 early_start_pfn = align_down(early_base, PAGE_SIZE) / PAGE_SIZE;
    u64 early_end_pfn = align_up(early_current, PAGE_SIZE) / PAGE_SIZE;

    /* Mark usable pages, but AVOID kernel and early_alloc regions. */
    struct limine_memmap_response *resp = memmap_request.response;
    if (!resp) return -1;

    for (u64 i = 0; i < resp->entry_count; i++)
    {
        struct limine_memmap_entry *e = resp->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;

        paddr_t base = align_up(e->base, PAGE_SIZE);
        paddr_t end = align_down(e->base + e->length, PAGE_SIZE);
        if (base >= end) continue;

        u64 start_pfn = base / PAGE_SIZE;
        u64 end_pfn = end / PAGE_SIZE;

        for (u64 pfn = start_pfn; pfn < end_pfn; pfn++)
        {
            /* Check if this PFN is in kernel or early_alloc range */
            if ((pfn >= kernel_start_pfn && pfn < kernel_end_pfn) ||
                (pfn >= early_start_pfn && pfn < early_end_pfn))
            {
                continue;
            }
            buddy.pages[pfn].flags = PAGE_FREE;
        }
    }

    /* Now group free pages into the largest possible blocks for the free lists. */
    for (u64 i = 0; i < buddy.page_count; )
    {
        if (buddy.pages[i].flags != PAGE_FREE)
        {
            i++;
            continue;
        }

        u8 order = MAX_ORDER;
        while (order > 0)
        {
            if (IS_ALIGNED(i, order) && (i + (1UL << order) <= buddy.page_count))
            {
                bool all_free = true;
                for (u64 j = 0; j < (1UL << order); j++)
                {
                    if (buddy.pages[i + j].flags != PAGE_FREE)
                    {
                        all_free = false;
                        break;
                    }
                }
                if (all_free) break;
            }
            order--;
        }

        buddy_add_block(&buddy.pages[i], order);
        i += (1UL << order);
    }

    return 0;
}

core_initcall(buddy_init);

vaddr_t buddy_alloc_vaddr(int order)
{
    if (order > MAX_ORDER) return 0;

    for (int i = order; i <= MAX_ORDER; i++)
    {
        if (list_empty(&buddy.free_areas[i].free_list)) continue;

        struct buddy_page *page = container_of(buddy.free_areas[i].free_list.next, struct buddy_page, list);
        list_del(&page->list);
        buddy.free_areas[i].free_count--;

        /* Split blocks if necessary */
        while (i > order)
        {
            i--;
            struct buddy_page *buddy_pg = &page[1UL << i];
            buddy_add_block(buddy_pg, i);
        }

        size_t pfn = page - buddy.pages;
        for (u64 j = 0; j < (1UL << order); j++)
        {
            buddy.pages[pfn + j].flags = PAGE_ALLOCATED;
            buddy.pages[pfn + j].order = order;
        }

        return (vaddr_t)((pfn * PAGE_SIZE) + hhdm_offset);
    }

    return 0;
}

vaddr_t buddy_alloc(int order)
{
    return buddy_alloc_vaddr(order);
}

void buddy_free(vaddr_t vaddr, int order)
{
    if (!vaddr) return;
    paddr_t paddr = vaddr - hhdm_offset;
    u64 pfn = paddr / PAGE_SIZE;

    if (pfn >= buddy.page_count) return;

    /* Proper buddy merging should be here, but for now just add it back */
    for (u64 i = 0; i < (1UL << order); i++)
    {
        buddy.pages[pfn + i].flags = PAGE_FREE;
    }
    buddy_add_block(&buddy.pages[pfn], order);
}