#ifndef PMM_H
#define PMM_H

#include <stdint.h>

typedef uint64_t block_id_t;

void kprint_ammount_mem_mb();
void pmm_init();
void *pmm_alloc_block();
void *pmm_alloc_block_row(uint64_t ammount);
void pmm_free_block(void *ptr);
uint64_t pmm_free_block_count();
uint64_t pmm_used_block_count();

extern uint64_t hhdm_offset;
extern uint64_t mem_ammount_b;
extern uint64_t usable_memory;

extern uint8_t *bitmap;
extern uint64_t total_blocks;
extern uint64_t usable_blocks;

#endif