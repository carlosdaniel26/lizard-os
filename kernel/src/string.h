#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <types.h>

int memcmp(const void *, const void *, size_t size);
int strcmp(const void *aptr, const void *bptr);
void *memcpy(void *dstptr, const void *srcptr, size_t size);
void *memmove(void *, const void *, size_t size);
void *memset(void *, int value, size_t size);
size_t strlen(const char *);
char *strchr(const char *str, int ch);
bool strsIsEqual(const char *str1, const char *str2, size_t size);
void unsigned_to_string(u64 value, char *str);
unsigned get_unsigned2string_final_size(u64 value);
unsigned get_u64tostring_final_size(u64 value);
long unsigned get_unsigned2hex_final_size(u64 value);
void unsigned_to_hexstring(u64 value, char *str);
char *strcpy(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
int sprintf(char *buffer, const char *format, ...);

