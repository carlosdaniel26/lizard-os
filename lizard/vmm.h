#pragma once

#include <nolibc/types.h>

extern vaddr_t current_pml4;

void vmm_map_page(vaddr_t vaddr, paddr_t paddr, u64 flags);
void vmm_unmap_page(vaddr_t vaddr);
void *vmm_alloc(vaddr_t pml4, vaddr_t vaddr, u64 flags);
void *vmm_alloc_page(void);
void vmm_free_page(vaddr_t vaddr);
void vmm_switch_pml4(vaddr_t pml4);
