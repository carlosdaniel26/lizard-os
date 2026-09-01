#pragma once

#include <lizard/pit.h>
#include <lizard/rtc.h>
#include <nolibc/stdarg.h>
#include <nolibc/stdbool.h>

#include <nolibc/types.h>
#include <lizard/vga.h>

extern u32 tty_color;

#define EOF (-1)
typedef int (*pfnStreamWriteBuf)(char *);

bool kprint(const char *data, size_t length);
int kprintf(const char *__restrict, ...);
int kvprintf(const char *__restrict, va_list);

int putchar(char character);
void dd(const char *restrict format, ...);