#pragma once
#include <stddef.h>

int putchar(int c);
int puts(const char *s);              /* writes s + '\n', like C stdio puts */
int printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
