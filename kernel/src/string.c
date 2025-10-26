#include <stdbool.h>
#include <types.h>
#include <string.h>
#include <stdarg.h>

int memcmp(const void *aptr, const void *bptr, size_t size)
{
	unsigned char *a = (unsigned char *)aptr;
	unsigned char *b = (unsigned char *)bptr;

	int diff = 0;

	for (size_t i = 0; i < size; i++)
	{
		if (a[i] != b[i])
			diff++;
	}
	return diff;
}

int strcmp(const void *aptr, const void *bptr)
{
	return memcmp(aptr, bptr, strlen(bptr));
}

void *memcpy(void *dstptr, const void *srcptr, size_t size)
{
	unsigned char *d = (unsigned char *)dstptr;
	unsigned char *s = (unsigned char *)srcptr;

	for (size_t i = 0; i < size; i++)
	{
		d[i] = s[i];
	}
	return dstptr;
}

void *memmove(void *dstptr, const void *srcptr, size_t size)
{
	unsigned char *dst = (unsigned char *)dstptr;
	const unsigned char *src = (const unsigned char *)srcptr;
	if (dst < src)
	{
		for (size_t i = 0; i < size; i++)
			dst[i] = src[i];
	} else
	{
		for (size_t i = size; i != 0; i--)
			dst[i - 1] = src[i - 1];
	}
	return dstptr;
}

void *memset(void *pointer, int value, size_t size)
{
	unsigned char *p = pointer;

	for (size_t i = 0; i < size; i++)
	{
		p[i] = (unsigned char)value;
	}

	return pointer;
}

size_t strlen(const char *str)
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
	do
	{
		if (*str == (char)ch)
			return (char *)str;
	} while (*str++);

	return NULL;
}

char *strcpy(char *dest, const char *src)
{
	char *ret = dest;
	while ((*dest++ = *src++) != '\0')
		;
	return ret;
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

void unsigned_to_string(u64 value, char *str)
{
	char buffer[20];
	int i = 0;

	if (value == 0)
	{
		str[0] = '0';
		str[1] = '\0';
		return;
	}

	/* convert in reverse order*/
	while (value > 0)
	{
		buffer[i++] = (value % 10) + '0'; /* (get last digit), convert to ASCII*/
		value /= 10;					  /* decrease number by one decimal case*/
	}

	/* reverse*/
	for (int j = 0; j < i; j++)
	{
		str[j] = buffer[i - j - 1];
	}
	str[i] = '\0'; /* null in the end*/
}

unsigned get_unsigned2string_final_size(u64 value)
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

unsigned get_u64tostring_final_size(u64 value)
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

unsigned get_unsigned2hex_final_size(u64 value)
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

void unsigned_to_hexstring(u64 value, char *str)
{
	const char *hex_digits = "0123456789abcdef";
	int index = 0;
	char buffer[16];

	if (value == 0)
	{
		str[0] = '0';
		str[1] = '\0';
		return;
	}

	while (value > 0 && index < 16)
	{
		buffer[index++] = hex_digits[value & 0xF];
		value >>= 4;
	}

	for (int i = 0; i < index; i++)
	{
		str[i] = buffer[index - i - 1];
	}
	str[index] = '\0';
}

char *strncpy(char *dest, const char *src, size_t n)
{
	size_t i;

	for (i = 0; i < n && src[i] != '\0'; i++)
	{
		dest[i] = src[i];
	}

	// Pad the rest with '\0' if src is shorter than n
	for (; i < n; i++)
	{
		dest[i] = '\0';
	}

	return dest;
}

char *strtok(char *str, const char *delim)
{
	static char *next;
	char *start;
	
	if (str)
		next = str;
	if (!next)
		return NULL;

	while (*next && strchr(delim, *next))
		next++;

	if (*next == '\0') {
		next = NULL;
		return NULL;
	}

	start = next;

	while (*next && !strchr(delim, *next))
		next++;

	if (*next) {
		*next = '\0';
		next++;
	} else {
		next = NULL;
	}

	return start;
}

char *strtok_r(char *str, const char *delim, char **saveptr)
{
	char *start;

	if (str)
		*saveptr = str;
	if (!*saveptr)
		return NULL;

	while (**saveptr && strchr(delim, **saveptr))
		(*saveptr)++;

	if (**saveptr == '\0') {
		*saveptr = NULL;
		return NULL;
	}

	start = *saveptr;

	while (**saveptr && !strchr(delim, **saveptr))
		(*saveptr)++;

	if (**saveptr) {
		**saveptr = '\0';
		(*saveptr)++;
	} else {
		*saveptr = NULL;
	}

	return start;
}

int sprintf(char *buffer, const char *format, ...)
{
	va_list args;
	va_start(args, format);

	char *buf_ptr = buffer;
	const char *fmt_ptr = format;

	while (*fmt_ptr) {
		if (*fmt_ptr == '%') {
			fmt_ptr++;
			if (*fmt_ptr == 's') {
				char *str = va_arg(args, char *);
				while (*str) {
					*buf_ptr++ = *str++;
				}
			} else if (*fmt_ptr == 'd') {
				int val = va_arg(args, int);
				char num_buffer[20];
				unsigned_to_string((val < 0) ? -val : val, num_buffer);
				if (val < 0) {
					*buf_ptr++ = '-';
				}
				char *num_ptr = num_buffer;
				while (*num_ptr) {
					*buf_ptr++ = *num_ptr++;
				}
			} else if (*fmt_ptr == 'x') {
				unsigned int val = va_arg(args, unsigned int);
				char hex_buffer[20];
				unsigned_to_hexstring(val, hex_buffer);
				char *hex_ptr = hex_buffer;
				while (*hex_ptr) {
					*buf_ptr++ = *hex_ptr++;
				}
			} else if (*fmt_ptr == 'c') {
				char ch = (char)va_arg(args, int);
				*buf_ptr++ = ch;
			} else {
				*buf_ptr++ = '%';
				*buf_ptr++ = *fmt_ptr;
			}
		} else {
			*buf_ptr++ = *fmt_ptr;
		}
		fmt_ptr++;
	}

	*buf_ptr = '\0';
	va_end(args);
	return buf_ptr - buffer;
}