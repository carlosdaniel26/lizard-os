#pragma once
#include <stddef.h>
#include <stdarg.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#define EOF (-1)
#define BUFSIZ 1024

typedef struct _FILE FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen(const char *path, const char *mode);
int   fclose(FILE *f);
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *f);
size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f);
int   fseek(FILE *f, long off, int whence);
long  ftell(FILE *f);
void  rewind(FILE *f);
int   feof(FILE *f);
int   ferror(FILE *f);
void  clearerr(FILE *f);
int   fflush(FILE *f);
int   fileno(FILE *f);
void  setbuf(FILE *f, char *buf);
int   setvbuf(FILE *f, char *buf, int mode, size_t size);

int   fgetc(FILE *f);
int   getc(FILE *f);
char *fgets(char *s, int size, FILE *f);
int   ungetc(int c, FILE *f);

int   fputc(int c, FILE *f);
int   putc(int c, FILE *f);
int   putchar(int c);
int   fputs(const char *s, FILE *f);
int   puts(const char *s);

int   printf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int   fprintf(FILE *f, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int   sprintf(char *buf, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
int   snprintf(char *buf, size_t n, const char *fmt, ...) __attribute__((format(printf, 3, 4)));
int   vprintf(const char *fmt, va_list ap);
int   vfprintf(FILE *f, const char *fmt, va_list ap);
int   vsprintf(char *buf, const char *fmt, va_list ap);
int   vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);

int   sscanf(const char *str, const char *fmt, ...);
int   fscanf(FILE *f, const char *fmt, ...);

int   remove(const char *path);
int   rename(const char *from, const char *to);
void  perror(const char *s);
