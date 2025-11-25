#pragma once

#include <types.h>

#define PAGE_PRESENT    (1 << 0)
#define PAGE_WRITABLE   (1 << 1)
#define PAGE_USER       (1 << 2)
#define PAGE_WRITETHROUGH (1 << 3)
#define PAGE_CACHE_DISABLE (1 << 4)
#define PAGE_ACCESSED   (1 << 5)
#define PAGE_DIRTY      (1 << 6)
#define PAGE_HUGE       (1 << 7)
#define PAGE_GLOBAL     (1 << 8)

#define PAGE_SIZE 4096

u64 *pgtable_alloc_table();
u64 *pgtable_free(u64 *pml4);
void pgtable_map(u64 *pml4, u64 virt, u64 phys, u64 flags);
void pgtable_unmap(u64 *pml4, u64 virt);
void pgtable_maprange(u64 *pml4, u64 virt, u64 phys, u64 length, u64 flags);
void pgtable_switch(u64 *pml4);
