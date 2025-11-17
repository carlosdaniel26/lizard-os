#include <buddy.h>
#include <types.h>
#include <string.h>
#include <stdio.h>
#include <limine.h>
#include <helpers.h>

/* function prototypes */
void *early_alloc(size_t pages);
void early_init(void);
MemoryRegion *buddy_parse_mmap(void);

extern u32 kernel_start;
extern u32 kernel_end;

#define KERNEL_STACK_SIZE 0x4000 /* 16 KiB */
extern u8 kernel_stack[KERNEL_STACK_SIZE];

extern struct limine_memmap_request memmap_request;
extern u64 hhdm_offset;

#define PAGE_SIZE 4096

static MemoryRegion *regions = NULL;
static size_t regions_count = 0;
static BuddyAllocator allocator;

/* 
 * early allocation explanation:
 *
 * initially as i dont want to depends on global variables to handle a linked list,
 * i'm going to use a bitmap allocator initially just to allocate this linked list,
 * after that, then i'll rellay on the buddy allocator futher on.
 * 
 * so the following functions, variables and macros will be related and used
 * on this early allocation before buddy be ready, which btw buddy will use to allocate 
 * its structures too.
 * 
 * that kinda remember me egg and chicken problem.
 */

#define EARLY_BITMAP_SIZE 8192  /* 8kb can track 64k pages (256mb) */

static unsigned long early_bitmap[EARLY_BITMAP_SIZE / sizeof(unsigned long)];
static uintptr_t early_memory_base;
static size_t early_pages;

/* check if a page is free in the early bitmap */
static int early_page_is_free(size_t page_index)
{
    size_t long_index = page_index / (sizeof(unsigned long) * 8);
    size_t bit_index = page_index % (sizeof(unsigned long) * 8);
    return !(early_bitmap[long_index] & (1UL << bit_index));
}

/* mark a page as used in the early bitmap */
static void mark_page_used(size_t page_index)
{
    size_t long_index = page_index / (sizeof(unsigned long) * 8);
    size_t bit_index = page_index % (sizeof(unsigned long) * 8);
    early_bitmap[long_index] |= (1UL << bit_index);
}

/* find consecutive free pages in early bitmap */
static int find_free_pages(size_t pages, size_t *start_index)
{
    size_t consecutive_free = 0;
    
    for (size_t i = 0; i < early_pages; i++)
    {
        if (early_page_is_free(i))
        {
            if (consecutive_free == 0)
            {
                *start_index = i;
            }
            consecutive_free++;
            if (consecutive_free >= pages)
            {
                return 1;
            }
        }
        else
        {
            consecutive_free = 0;
        }
    }
    return 0;
}

/* count usable memory regions from limine memory map */
size_t count_usable_regions(void)
{
    struct limine_memmap_response *response = memmap_request.response;
    size_t count = 0;
    
    if (!response) return 0;
    
    for (size_t i = 0; i < response->entry_count; i++)
    {
        if (response->entries[i]->type == LIMINE_MEMMAP_USABLE)
        {
            count++;
        }
    }
    return count;
}

/* parse the memory map from limine and populate regions linked list */
MemoryRegion *buddy_parse_mmap(void)
{
    struct limine_memmap_response *response = memmap_request.response;
    if (!response || !response->entry_count)
    {
        return NULL;
    }

    MemoryRegion *head = NULL;
    MemoryRegion *tail = NULL;
    regions_count = 0;

    for (size_t i = 0; i < response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = response->entries[i];

        u64 aligned_base = align_up(entry->base, PAGE_SIZE);
        u64 aligned_end = align_down(entry->base + entry->length, PAGE_SIZE);

        if (aligned_base >= aligned_end)
        {
            continue;
        }

        u64 aligned_length = aligned_end - aligned_base;
        
        /* allocate memory for this region node using early allocator */
        size_t pages_needed = (sizeof(MemoryRegion) + PAGE_SIZE - 1) / PAGE_SIZE;
        MemoryRegion *region = early_alloc(pages_needed) + hhdm_offset;
        if (!region) break;

        region->base = aligned_base;
        region->length = aligned_length;
        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            region->type = MEMORY_REGION_USABLE;
        }
        else
        {
            region->type = MEMORY_REGION_RESERVED;
        }
        region->next = NULL;

        /* add to linked list */
        if (!head)
        {
            head = region;
            tail = region;
        }
        else
        {
            tail->next = region;
            tail = region;
        }
        
        regions_count++;
    }

    regions = head;
    return head;
}

/* initialize early bitmap allocator */
void early_init(void)
{
    struct limine_memmap_response *response = memmap_request.response;
    if (!response || !response->entry_count)
    {
        return;
    }

    /* find first usable region for early allocation */
    for (size_t i = 0; i < response->entry_count; i++)
    {
        if (response->entries[i]->type == LIMINE_MEMMAP_USABLE)
        {
            early_memory_base = response->entries[i]->base;
            early_pages = response->entries[i]->length / PAGE_SIZE;
            break;
        }
    }

    /* initialize bitmap - all pages free initially */
    memset(early_bitmap, 0, sizeof(early_bitmap));
	
}

/* allocate pages using early bitmap allocator */
void *early_alloc(size_t pages)
{
    size_t start_index;
    
    if (!find_free_pages(pages, &start_index))
    {
        return NULL;
    }

    /* mark pages as used */
    for (size_t i = 0; i < pages; i++)
    {
        mark_page_used(start_index + i);
    }

    return (void *)(early_memory_base + (start_index * PAGE_SIZE));
}

/* dump all memory regions for debugging */
void buddy_dump_regions(MemoryRegion *regions)
{
    kprintf("memory regions:\n");
    MemoryRegion *current = regions;
    size_t i = 0;
    while (current)
    {
        if (current->type == MEMORY_REGION_USABLE)
        {
            kprintf("base: 0x%x, length: 0x%x, type: usable\n", current->base, current->length);
        }
        else
        {
            kprintf("base: 0x%x, length: 0x%x, type: reserved\n", current->base, current->length);
        }
        current = current->next;
        i++;
    }
}

/* calculate total usable memory across all regions */
size_t buddy_calculate_usable_memory(MemoryRegion *regions)
{
    size_t total = 0;
    MemoryRegion *current = regions;
    while (current)
    {
        if (current->type == MEMORY_REGION_USABLE)
        {
            total += current->length;
        }
        current = current->next;
    }
    return total;
}

/* initialize buddy allocator */
void buddy_init(void)
{
    early_init();
	
    regions = buddy_parse_mmap();
    
	if (!regions)
    {
        return;
    }

    buddy_dump_regions(regions);
	
    size_t total_memory = buddy_calculate_usable_memory(regions);
    kprintf("total usable memory: %u mb\n", total_memory / (1024 * 1024));
    memset(&allocator, 0, sizeof(allocator));

    /* use first usable region for buddy allocator */
    MemoryRegion *current = regions;
    while (current)
    {
        if (current->type == MEMORY_REGION_USABLE)
        {
            allocator.base = current->base;
            allocator.size = current->length & ~(PAGE_SIZE - 1); /* align down to page size */
            break;
        }
        current = current->next;
    }

    /* initialize free lists for each order */
    size_t block_size = PAGE_SIZE; /* start with 4kb blocks */
    u64 addr = allocator.base;

    for (int order = 0; order < MAX_ORDER && block_size <= allocator.size; order++)
    {
        BuddyBlock *prev = NULL;
        /* add all blocks of current size to free list */
        while (addr + block_size <= allocator.base + allocator.size)
        {
            BuddyBlock *block = (BuddyBlock *)(addr + hhdm_offset); /* convert to virtual address */
            block->next = prev;
            prev = block;
            addr += block_size;
        }
        allocator.free_list[order] = prev;
        block_size <<= 1; /* double block size for next order */
    }
}

/* allocate a block of 2^order pages */
void *buddy_alloc(int order)
{
    if (order >= MAX_ORDER)
    {
        return NULL;
    }

    /* search for first available block of sufficient size */
    for (int current = order; current < MAX_ORDER; current++)
    {
        if (allocator.free_list[current])
        {
            BuddyBlock *block = allocator.free_list[current];
            allocator.free_list[current] = block->next;

            /* split larger blocks until we reach requested order */
            while (current > order)
            {
                current--;
                /* calculate buddy address and add to free list */
                BuddyBlock *buddy = (BuddyBlock *)((u64)block + (1ULL << (current + 12))); /* +12 for page size in bits */
                buddy->next = allocator.free_list[current];
                allocator.free_list[current] = buddy;
            }

            return (void *)((u64)block - hhdm_offset); /* convert back to physical address */
        }
    }

    return NULL; /* no memory available */
}

/* free a block back to buddy allocator */
void buddy_free(void *ptr, int order)
{
    u64 addr = (u64)ptr + hhdm_offset;
    BuddyBlock *block = (BuddyBlock *)addr;

    while (order < MAX_ORDER - 1)
    {
        u64 buddy_addr = ((addr - allocator.base) ^ (1ULL << (order + 12))) + allocator.base;
        BuddyBlock **prev = &allocator.free_list[order];
        BuddyBlock *current = allocator.free_list[order];
        int merged = 0;

        while (current)
        {
            if ((u64)current == buddy_addr + hhdm_offset)
            {
                if (*prev == current)
                {
                    *prev = current->next;
                }
                else
                {
                    BuddyBlock *tmp = allocator.free_list[order];
                    while (tmp && tmp->next != current)
                    {
                        tmp = tmp->next;
                    }
                    if (tmp)
                    {
                        tmp->next = current->next;
                    }
                }
                if (addr < buddy_addr)
                {
                    addr = addr;
                }
                else
                {
                    addr = buddy_addr;
                }
                block = (BuddyBlock *)addr;
                order++;
                merged = 1;
                break;
            }
            prev = &current->next;
            current = current->next;
        }

        if (!merged)
        {
            break;
        }
    }

    block->next = allocator.free_list[order];
    allocator.free_list[order] = block;
}

MemoryRegion *buddy_get_regions()
{
    return regions;
}