#include <stdint.h>
#include <string.h>
#include <stdbool.h>

int memcmp(const void* aptr, const void* bptr, size_t size)
{
	unsigned char* a = (unsigned char*) aptr;
	unsigned char* b = (unsigned char*) bptr;

	int diff = 0;

	for (size_t i = 0; i < size; i++)
	{
		if (a[i] != b[i])
			diff++;
	}
	return diff;
}

int strcmp(const void* aptr, const void* bptr)
{
	return memcmp(aptr, bptr, strlen(bptr));
}

void* memcpy(void* dstptr, const void* srcptr, size_t size)
{
	unsigned char* d = (unsigned char*) dstptr;
	unsigned char* s = (unsigned char*) srcptr;

	for (size_t i = 0; i < size; i++)
	{
		d[i] = s[i];
	}
	return dstptr;
}

void* memmove(void* dstptr, const void* srcptr, size_t size)
{
	unsigned char* dst = (unsigned char*) dstptr;
	const unsigned char* src = (const unsigned char*) srcptr;
	if (dst < src) {
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else {
		for (size_t i = size; i != 0; i--)
			dst[i-1] = src[i-1];
	}
	return dstptr;
}

void* memset(void* pointer, int value, size_t size)
{
	unsigned char *p = pointer;

	for (size_t i = 0; i < size; i++)
	{
		p[i] = (unsigned char)value;
	}

	return pointer;
}

size_t strlen(const char* str)
{
	size_t len = 0;
	while (str[len])
	{
		len++;
	}
	return len;
}

char *strchr(const char *str, int ch)
{
    do {
        if (*str == (char)ch)
            return (char *)str;
    } while (*str++);
    
    return NULL;
}