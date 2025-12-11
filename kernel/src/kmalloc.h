#pragma once

#include <types.h>
#include <string.h>

void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t size);
void *krealloc(void *ptr, size_t new_size);
