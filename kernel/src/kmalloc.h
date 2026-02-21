#pragma once

#include <types.h>
#include <string.h>

void kmalloc_init(void);
void *kmalloc(size_t size);
void kfree(void *ptr);
void *zalloc(size_t size);

void *krealloc(void *ptr, size_t old_size, size_t new_size);
void *zrealloc(void *ptr, size_t old_size, size_t new_size);
