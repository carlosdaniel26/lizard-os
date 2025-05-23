#ifndef PMM_H
#define PMM_H

#include <stdint.h>

void kprint_ammount_mem_mb();
void pmm_init();
void *pmm_alloc_block(uint32_t ammount);
void pmm_reserve_block(uint32_t block_number);
void pmm_free_block(void* ptr);
uint32_t pmm_free_block_count();

#endif