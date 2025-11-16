#pragma once

#include <pmm.h>

#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

#define VIRT_ADDR(phys) (phys + hhdm_offset)

void vmm_init();
void *vmm_alloc_page(void *pml4);
void *vmm_alloc_block_row(void *pml4, u64 ammount);
int vmm_free_page(u64 *pml4, uintptr_t ptr);
