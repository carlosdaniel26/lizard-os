#pragma once

#include <types.h>

extern u64 *current_pml4;

void vmm_init(void);
void vmm_map_page(u64 virt, u64 phys, u64 flags);
void vmm_unmap_page(u64 virt);
void *vmm_alloc_page(void);
void vmm_free_page(void *ptr);
void vmm_switch_pml4(u64 *pml4);
