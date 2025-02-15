#include <stddef.h>
#include <stdbool.h>

#define EOF (-1)
typedef int (*pfnStreamWriteBuf)(char*);

struct Stream {
	size_t buf_len;
	size_t buf_i;
	char *buf;
	pfnStreamWriteBuf pfn_write_all;
};

bool print(const char* data, size_t length);
int printf(const char* __restrict, ...);
int putchar(char character);
int puts(const char*);