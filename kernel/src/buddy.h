#pragma once

#include <limine.h>
#include <types.h>
#include <string.h>

#define MAX_ORDER 16

#define MEMORY_REGION_USABLE 0
#define MEMORY_REGION_RESERVED 1

typedef struct MemoryRegion {
    uintptr_t base;
    size_t length;
    uint32_t type;
    uint32_t reserved;
    struct MemoryRegion *next;
} MemoryRegion;

typedef struct BuddyBlock
{
    struct BuddyBlock *next;
}
BuddyBlock;

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
    BuddyBlock *free_list[MAX_ORDER];
    u64 base;        /* physical base address of managed memory */
    size_t size;     /* total size of managed memory in bytes */
}
BuddyAllocator;

MemoryRegion *buddy_parse_mmap(void);
void buddy_dump_regions(MemoryRegion *regions);
size_t buddy_calculate_usable_memory(MemoryRegion *regions);
