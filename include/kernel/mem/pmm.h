#ifndef PMM_H
#define PMM_H

void kprint_ammount_mem_mb();
void pmm_init();
void *pmm_alloc_block();
void pmm_free_block(void* ptr);

#endif