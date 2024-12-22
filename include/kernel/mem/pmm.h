#ifndef PMM_H
#define PMM_H

#include <multiboot2.h>

void detect_memory(struct multiboot_info_t* mb_info);
void print_ammount_mem_mb();
void test_pmm();
void pmm_init(struct multiboot_info_t* mb_info);
void *pmm_alloc_block();
void pmm_free_block(void* ptr);
void process_memory_map(const struct multiboot_info_t *mb_info);

#endif