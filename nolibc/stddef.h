#pragma once

#include <nolibc/types.h>

typedef __PTRDIFF_TYPE__ ptrdiff_t;
typedef __WCHAR_TYPE__ wchar_t;

#define PAGE_SIZE 4096UL
#define __page_aligned __attribute__((aligned(PAGE_SIZE)))
#define __aligned(x) __attribute__((aligned(x)))
