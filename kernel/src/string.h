#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

int memcmp(const void*, const void*, size_t size);
int strcmp(const void* aptr, const void* bptr);
void* memcpy(void* __restrict, const void* __restrict, size_t size);
void* memmove(void*, const void*, size_t size);
void* memset(void*, int value, size_t size);
size_t strlen(const char* str);

#endif
