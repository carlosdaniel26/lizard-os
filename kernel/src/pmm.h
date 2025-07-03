#ifndef PMM_H
#define PMM_H

#include <stdint.h>

typedef uint64_t block_id_t;

void kprint_ammount_mem_mb();
void pmm_init();
void *pmm_alloc_block();
void pmm_reserve_block(block_id_t block_number);
void pmm_free_block(void* ptr);
block_id_t pmm_free_block_count();
void pmm_test_all();

#endif