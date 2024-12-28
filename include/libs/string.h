#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

int memcmp(const void*, const void*, size_t size);
void* memcpy(void* __restrict, const void* __restrict, size_t size);
void* memmove(void*, const void*, size_t size);
void* memset(void*, int value, size_t size);
size_t strlen(const char*);
bool strsIsEqual(const char *str1, const char *str2, size_t size);
void unsigned_to_string(uint64_t value, char *str);
unsigned get_unsigned2string_final_size(uint64_t value);
unsigned get_u64tostring_final_size(uint64_t value);
void unsigned_to_hexstring(uint64_t value, char *str);
#endif