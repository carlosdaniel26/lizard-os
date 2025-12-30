#pragma once

#include <limine.h>
#include <types.h>
#include <string.h>
#include <limine.h>
#include <list.h>

#define MAX_ORDER 16

#define PAGE_FREE     (1 << 0)
#define PAGE_RESERVED (1 << 1)
#define PAGE_ALLOCATED (1 << 2)

typedef struct BuddyPage {
    ListHead list; /* for linking free pages */
    u8 order;
    u8 flags;
} BuddyPage;

typedef struct BuddyArea {
    ListHead free_list;
    int free_count;
} BuddyArea;

typedef struct
{
    /* free lists for each order: 
     * order 0: 4kb blocks (1 page)
     * order 1: 8kb blocks (2 pages) 
     * order 2: 16kb blocks (4 pages)
     * order 3: 32kb blocks (8 pages)
     * order 4: 64kb blocks (16 pages)
     * order 5: 128kb blocks (32 pages)
     * order 6: 256kb blocks (64 pages)
     * order 7: 512kb blocks (128 pages)
     * order 8: 1mb blocks (256 pages)
     * order 9: 2mb blocks (512 pages)
     * order 10: 4mb blocks (1024 pages)
     * order 11: 8mb blocks (2048 pages)
     * order 12: 16mb blocks (4096 pages)
     * order 13: 32mb blocks (8192 pages)
     * order 14: 64mb blocks (16384 pages)
     * order 15: 128mb blocks (32768 pages) */

    BuddyPage *pages; /* array descriptor of all pages */
    size_t page_count;

    BuddyArea free_areas[MAX_ORDER + 1];
} BuddyAllocator;

extern BuddyAllocator buddy;

void buddy_init();
void *buddy_alloc(int order);
void buddy_free(void *addr, int order);
unsigned int pages_to_order(unsigned int pages);

extern u64 hhdm_offset;