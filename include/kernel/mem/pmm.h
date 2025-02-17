#ifndef PMM_H
#define PMM_H

#include <multiboot2.h>

void kprint_ammount_mem_mb();
void test_pmm();
void pmm_init();
void *pmm_alloc_block();
void pmm_free_block(void* ptr);

#endif