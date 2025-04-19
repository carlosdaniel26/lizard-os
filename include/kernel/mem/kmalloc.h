#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdbool.h>

typedef struct KMemoryHeader {
    struct KMemoryHeader* prev;
    struct KMemoryHeader* next;
    size_t size;
    bool is_free;
    
} KMemoryHeader __attribute__((packed));

void* kmalloc(size_t n_bytes);
void kfree(void* ptr);

#endif