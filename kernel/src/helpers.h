#pragma once

#include <types.h>

#define BIT_SET(byte, index) (byte |= (1 << ((index) % 8)))
#define BIT_CLEAR(byte, index) (byte &= ~(1 << ((index) % 8)))
#define BIT_TEST(byte, index) (byte & (1 << ((index) % 8)))

#define align_ptr_down(ptr, align) ((void *)((uintptr_t)(ptr) & ~(align - 1)))

#define align_ptr_up(ptr, align) ((void *)((((uintptr_t)(ptr)) + (align - 1)) & ~(align - 1)))

#define align_down(num, align) (num - (num % align))
#define align_up(num, align) (num + (align - (num % align)))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

char toupper(char c);
void hlt();
int oct2bin(unsigned char *str, int size);
char toupper(char c);
int days_in_month(int month, int year);