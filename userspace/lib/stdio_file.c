/* FILE* layer: a thin, unbuffered wrapper over the fd syscalls. Pulled in only
 * by programs that do file I/O (so it, and the heap it needs, stay out of tiny
 * programs that only printf). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <abi/syscall.h>
#include <sys/syscall.h>
#include "libc_internal.h"

struct _FILE {
    int fd;
    int eof;
    int err;
    int ungot; /* -1 when empty */
    int is_std;
};

static FILE s_stdin  = { 0, 0, 0, -1, 1 };
static FILE s_stdout = { 1, 0, 0, -1, 1 };
static FILE s_stderr = { 2, 0, 0, -1, 1 };

FILE *stdin  = &s_stdin;
FILE *stdout = &s_stdout;
FILE *stderr = &s_stderr;

FILE *fopen(const char *path, const char *mode)
{
    int flags = 0, rd = 0, wr = 0;

    for (const char *m = mode; *m; m++) {
        if (*m == 'r') rd = 1;
        else if (*m == 'w') { wr = 1; flags |= O_CREAT | O_TRUNC; }
        else if (*m == 'a') { wr = 1; flags |= O_CREAT | O_APPEND; }
        else if (*m == '+') rd = wr = 1;
    }
    if (rd && wr) flags |= O_RDWR;
    else if (wr)  flags |= O_WRONLY;
    else          flags |= O_RDONLY;

    int fd = sys_open(path, flags);
    if (fd < 0) {
        errno = ENOENT;
        return NULL;
    }

    FILE *f = malloc(sizeof(FILE));
    if (!f) {
        sys_close(fd);
        return NULL;
    }
    f->fd = fd;
    f->eof = f->err = 0;
    f->ungot = -1;
    f->is_std = 0;
    return f;
}

int fclose(FILE *f)
{
    if (!f || f->is_std)
        return 0;
    int r = sys_close(f->fd);
    free(f);
    return r < 0 ? EOF : 0;
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *f)
{
    if (!f || size == 0 || nmemb == 0)
        return 0;

    unsigned char *out = ptr;
    size_t total = size * nmemb, got = 0;

    if (f->ungot >= 0) {
        out[got++] = (unsigned char)f->ungot;
        f->ungot = -1;
    }
    while (got < total) {
        long n = sys_read(f->fd, out + got, total - got);
        if (n < 0) { f->err = 1; break; }
        if (n == 0) { f->eof = 1; break; }
        got += (size_t)n;
    }
    return got / size;
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *f)
{
    if (!f || size == 0 || nmemb == 0)
        return 0;

    const unsigned char *in = ptr;
    size_t total = size * nmemb, put = 0;

    while (put < total) {
        long n = sys_write(f->fd, in + put, total - put);
        if (n <= 0) { f->err = 1; break; }
        put += (size_t)n;
    }
    return put / size;
}

int fseek(FILE *f, long off, int whence)
{
    if (!f)
        return -1;
    f->ungot = -1;
    f->eof = 0;
    return sys_lseek(f->fd, off, whence) < 0 ? -1 : 0;
}

long ftell(FILE *f)
{
    if (!f)
        return -1;
    long r = sys_lseek(f->fd, 0, LZ_SEEK_CUR);
    if (r >= 0 && f->ungot >= 0)
        r--;
    return r;
}

void rewind(FILE *f)          { fseek(f, 0, SEEK_SET); if (f) f->err = 0; }
int feof(FILE *f)             { return f ? f->eof : 1; }
int ferror(FILE *f)           { return f ? f->err : 1; }
void clearerr(FILE *f)        { if (f) f->eof = f->err = 0; }
int fflush(FILE *f)           { (void)f; return 0; }
int fileno(FILE *f)           { return f ? f->fd : -1; }
void setbuf(FILE *f, char *b) { (void)f; (void)b; }
int setvbuf(FILE *f, char *b, int m, size_t sz) { (void)f; (void)b; (void)m; (void)sz; return 0; }

int fgetc(FILE *f)
{
    if (!f)
        return EOF;
    if (f->ungot >= 0) {
        int r = f->ungot;
        f->ungot = -1;
        return r;
    }
    unsigned char c;
    long n = sys_read(f->fd, &c, 1);
    if (n == 1)
        return c;
    if (n == 0) f->eof = 1;
    else        f->err = 1;
    return EOF;
}

int getc(FILE *f) { return fgetc(f); }

int ungetc(int c, FILE *f)
{
    if (!f || c == EOF)
        return EOF;
    f->ungot = (unsigned char)c;
    f->eof = 0;
    return c;
}

char *fgets(char *s, int size, FILE *f)
{
    if (size <= 0 || !f)
        return NULL;
    int i = 0;
    while (i < size - 1) {
        int c = fgetc(f);
        if (c == EOF)
            break;
        s[i++] = (char)c;
        if (c == '\n')
            break;
    }
    if (i == 0)
        return NULL;
    s[i] = '\0';
    return s;
}

int fputc(int c, FILE *f)
{
    unsigned char b = (unsigned char)c;
    return fwrite(&b, 1, 1, f) == 1 ? c : EOF;
}

int putc(int c, FILE *f) { return fputc(c, f); }

int fputs(const char *s, FILE *f)
{
    size_t n = strlen(s);
    return fwrite(s, 1, n, f) == n ? 0 : EOF;
}

int remove(const char *path)              { (void)path; return -1; }
int rename(const char *a, const char *b)  { (void)a; (void)b; return -1; }

void perror(const char *s)
{
    if (s && *s) {
        fputs(s, stderr);
        fputs(": ", stderr);
    }
    fputs("error\n", stderr);
}

/* ---- formatted I/O on FILE* --------------------------------------- */

int vfprintf(FILE *f, const char *fmt, va_list ap)
{
    char buf[1024];
    int r = __vfmt_buf(buf, sizeof(buf), fmt, ap);
    size_t n = (size_t)r < sizeof(buf) - 1 ? (size_t)r : sizeof(buf) - 1;
    fwrite(buf, 1, n, f);
    return r;
}

int fprintf(FILE *f, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vfprintf(f, fmt, ap);
    va_end(ap);
    return r;
}

/* ---- fscanf ------------------------------------------------------- */

static int file_get(void *v)   { return fgetc((FILE *)v); }
static void file_unget(void *v, int c) { ungetc(c, (FILE *)v); }

int fscanf(FILE *f, const char *fmt, ...)
{
    struct src sc = { .get = file_get, .unget = file_unget, .ctx = f };
    va_list ap;
    va_start(ap, fmt);
    int r = __vscan_core(&sc, fmt, ap);
    va_end(ap);
    return r;
}
