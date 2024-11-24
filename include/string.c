#include <string.h>
#include <stdint.h>

int memcmp(const void* aptr, const void* bptr, size_t size)
{
    unsigned char* a = (unsigned char*) aptr;
    unsigned char* b = (unsigned char*) bptr;

    for (size_t i = 0; i < size; i++)
    {
        if (a[i] != b[i])
            return 0;
        
    }
    return 1;
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

void unsigned_to_string(uint64_t value, char *str) 
{
    char buffer[20];
    int i = 0;

    if (value == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }

    // convert in reverse order
    while (value > 0) {
        buffer[i++] = (value % 10) + '0';   // (get last digit), convert to ASCII
        value /= 10;                        // decrease number by one decimal case 
    }

    // reverse
    for (int j = 0; j < i; j++) {
        str[j] = buffer[i - j - 1];
    }
    str[i] = '\0'; // null in the end
}