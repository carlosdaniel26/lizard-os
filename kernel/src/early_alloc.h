#pragma once

#include <stddef.h>
#include <limine.h>
#include <types.h>

void early_alloc_init();
void *early_alloc(size_t size, size_t align);
extern u64 highest_addr;

extern volatile struct limine_memmap_request memmap_request;

extern uintptr_t early_base;
extern uintptr_t early_end;
extern uintptr_t early_current;

extern u64 hhdm_offset;