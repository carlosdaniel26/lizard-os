#pragma once
#include <stddef.h>
#include <stdarg.h>

/* printf.c */
int __vfmt_buf(char *buf, size_t n, const char *fmt, va_list ap);

/* scanf.c */
struct src {
    int (*get)(void *);
    void (*unget)(void *, int);
    void *ctx;
};
int __vscan_core(struct src *sc, const char *fmt, va_list ap);
