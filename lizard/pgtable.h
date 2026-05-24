#pragma once

#include <lizard/memory_map.h>
#include <nolibc/stddef.h>
#include <nolibc/types.h>

#define PAGE_WRITETHROUGH (1 << 3)
#define PAGE_CACHE_DISABLE (1 << 4)
#define PAGE_ACCESSED (1 << 5)
#define PAGE_DIRTY (1 << 6)
#define PAGE_HUGE (1 << 7)
#define PAGE_GLOBAL (1 << 8)

u64 *pgtable_create(void);
u64 *pgtable_alloc_table(void);
u64 *pgtable_free(u64 *pml4);

void pgtable_map(u64 *pml4, u64 virt, u64 phys, u64 flags);
void pgtable_unmap(u64 *pml4, u64 virt);
void pgtable_maprange(u64 *pml4, u64 virt, u64 phys, u64 length, u64 flags);
void pgtable_switch(u64 *pml4);
int pgtable_is_mapped(u64 *pml4, u64 virt);