#ifndef KMALLOC_H
#define KMALLOC_H

#include <stdbool.h>
#include <stddef.h>

typedef struct __attribute__((packed)) KMemoryHeader {
    struct KMemoryHeader *prev;
    struct KMemoryHeader *next;
    size_t size;
    bool is_free;

} KMemoryHeader;

void *kmalloc(size_t n_bytes);
void *kcalloc(size_t n_bytes);
void *krealloc(void *ptr, size_t n_bytes);
void kfree(void *ptr);

#endif