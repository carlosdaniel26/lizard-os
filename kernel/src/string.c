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

bool strsIsEqual(const char *str1, const char *str2, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		if (str1[i] != str2[i])
			return 0;
	}

	return 1;
}

void unsigned_to_string(uint64_t value, char *str)
{
	char buffer[20];
	int i = 0;

	if (value == 0) {
		str[0] = '0';
		str[1] = '\0';
		return;
	}

	/* convert in reverse order*/
	while (value > 0) {
		buffer[i++] = (value % 10) + '0';   /* (get last digit), convert to ASCII*/
		value /= 10;						/* decrease number by one decimal case*/
	}

	/* reverse*/
	for (int j = 0; j < i; j++) {
		str[j] = buffer[i - j - 1];
	}
	str[i] = '\0'; /* null in the end*/
}

unsigned get_unsigned2string_final_size(uint64_t value)
{
	unsigned i = 0;

	if (value == 0)
		return 1;

	while (value > 0)
	{
		value /= 10;
		i++;
	}

	return i;
}

unsigned get_u64tostring_final_size(uint64_t value)
{
	unsigned i = 0;

	if (value == 0)
		return 1;

	while (value > 0)
	{
		value /= 10;
		i++;
	}

	return i;
}

unsigned get_unsigned2hex_final_size(uint64_t value)
{
	unsigned i = 0;

	if (value == 0)
		return 1;

	while (value > 0)
	{
		value /= 16;
		i++;
	}

	return i;
}

void unsigned_to_hexstring(uint64_t value, char *str)
{
	const char *hex_digits = "0123456789abcdef";
	int index = 0;
	char buffer[16];

	if (value == 0) {
		str[0] = '0';
		str[1] = '\0';
		return;
	}

	while (value > 0 && index < 16) {
		buffer[index++] = hex_digits[value & 0xF];
		value >>= 4;
	}

	for (int i = 0; i < index; i++) {
		str[i] = buffer[index - i - 1];
	}
	str[index] = '\0';
}
