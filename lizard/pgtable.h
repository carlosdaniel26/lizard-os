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

vaddr_t pgtable_create(void);
vaddr_t pgtable_alloc_table(void);
void pgtable_free(vaddr_t pml4);

void pgtable_map(vaddr_t pml4, vaddr_t vaddr, paddr_t paddr, u64 flags);
void pgtable_unmap(vaddr_t pml4, vaddr_t vaddr);
void pgtable_maprange(vaddr_t pml4, vaddr_t vaddr, paddr_t paddr, u64 length, u64 flags);
void pgtable_switch(vaddr_t pml4);
int pgtable_is_mapped(vaddr_t pml4, vaddr_t vaddr);