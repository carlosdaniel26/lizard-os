#include <stddef.h>
#include <stdbool.h>

#define EOF (-1)
typedef int (*pfnStreamWriteBuf)(char*);

bool kprint(const char* data, size_t length);
int kprintf(const char* __restrict, ...);
int putchar(char character);
int puts(const char*);