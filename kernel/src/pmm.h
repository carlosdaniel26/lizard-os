#pragma once

#include <types.h>

typedef u64 block_id_t;

void kprint_ammount_mem_mb();
void pmm_init();
void *pmm_alloc_block();
void *pmm_alloc_block_row(u64 ammount);
void pmm_free_block(void *ptr);
void pmm_free_block_row(void *ptr, u64 ammount);
u64 pmm_free_block_count();
u64 pmm_used_block_count();

extern u64 hhdm_offset;
extern u64 mem_ammount_b;
extern u64 usable_memory;

extern u8 *bitmap;
extern u64 total_blocks;
extern u64 usable_blocks;

