#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <vga.h>

extern uint32_t terminal_color;

#define EOF (-1)
typedef int (*pfnStreamWriteBuf)(char *);

bool kprint(const char *data, size_t length);
int kprintf(const char *__restrict, ...);
#define debug_printf(fmt, ...)																	   \
	kprintf("[DEBUG] %s:%u: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

void kpanic(const char *str);
int putchar(char character);
void dd(const char *restrict format, ...);