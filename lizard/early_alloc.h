#pragma once

#include <nolibc/types.h>

vaddr_t early_alloc(size_t size, size_t align);
extern u64 highest_addr;

extern vaddr_t early_base;
extern vaddr_t early_end;
extern vaddr_t early_current;

extern u64 hhdm_offset;