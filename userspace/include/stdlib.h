#pragma once
#include <stddef.h>

void *malloc(size_t size);
void *calloc(size_t n, size_t size);
void *realloc(void *ptr, size_t size);
void  free(void *ptr);

__attribute__((noreturn)) void exit(int code);
__attribute__((noreturn)) void abort(void);
int   atexit(void (*fn)(void));

int    atoi(const char *s);
long   atol(const char *s);
double atof(const char *s);
long  strtol(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);

int   abs(int x);
long  labs(long x);

void  qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));

int   rand(void);
void  srand(unsigned seed);
#define RAND_MAX 0x7fffffff

char *getenv(const char *name);
int   system(const char *cmd);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
