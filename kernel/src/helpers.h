#pragma once

#include <types.h>
#include <stddef.h>

#define BIT_SET(byte, index) (byte |= (1 << ((index) % 8)))
#define BIT_CLEAR(byte, index) (byte &= ~(1 << ((index) % 8)))
#define BIT_TEST(byte, index) (byte & (1 << ((index) % 8)))

#define align_ptr_down(ptr, align) ((void *)((uintptr_t)(ptr) & ~(align - 1)))

#define align_ptr_up(ptr, align) ((void *)((((uintptr_t)(ptr)) + (align - 1)) & ~(align - 1)))
#define is_ptr_aligned(ptr, align) ((((uintptr_t)(ptr)) & (align - 1)) == 0)

#define align_down(num, align) (num - (num % align))

static inline uintptr_t align_up(uintptr_t x, size_t a)
{
    return (x + a - 1) & ~(a - 1);
}

#define is_aligned(num, align) ((num % align) == 0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

char toupper(char c);
void hlt();
int oct2bin(unsigned char *str, int size);
char toupper(char c);
int days_in_month(int month, int year);