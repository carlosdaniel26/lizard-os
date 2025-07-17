#ifndef PMM_H
#define PMM_H

#include <stdint.h>

typedef uint64_t block_id_t;

void kprint_ammount_mem_mb();
void pmm_init();
void *pmm_alloc_block();
void *pmm_alloc_block_row(uint64_t ammount);
void pmm_free_block(void *ptr);
block_id_t pmm_free_block_count();

extern uint64_t hhdm_offset;

#endif