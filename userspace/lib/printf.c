/* Formatted output core: vfmt() drives a sink that is either a caller buffer
 * (the s*printf family) or a file descriptor (printf/puts). fprintf/FILE* live
 * in stdio_file.c so that a program using only printf doesn't drag in the heap. */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/syscall.h>

struct sink {
    char  *buf;    /* caller buffer, or NULL         */
    size_t cap;    /* capacity incl. NUL slot        */
    size_t len;    /* chars that would be written    */
    int    fd;     /* target fd, or -1               */
    char   fbuf[128];
    size_t fn;
};

static void sink_flush(struct sink *s)
{
    if (s->fd >= 0 && s->fn) {
        sys_write(s->fd, s->fbuf, s->fn);
        s->fn = 0;
    }
}

static void sink_putc(struct sink *s, char c)
{
    if (s->buf) {
        if (s->len + 1 < s->cap)
            s->buf[s->len] = c;
    } else if (s->fd >= 0) {
        if (s->fn == sizeof(s->fbuf))
            sink_flush(s);
        s->fbuf[s->fn++] = c;
    }
    s->len++;
}

static void sink_write(struct sink *s, const char *p, size_t n)
{
    for (size_t i = 0; i < n; i++)
        sink_putc(s, p[i]);
}

static void sink_pad(struct sink *s, char c, int n)
{
    while (n-- > 0)
        sink_putc(s, c);
}

static int u_to_str(unsigned long long v, unsigned base, int upper, char *out)
{
    const char *digs = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char tmp[32];
    int i = 0;
    do {
        tmp[i++] = digs[v % base];
        v /= base;
    } while (v);
    for (int j = 0; j < i; j++)
        out[j] = tmp[i - 1 - j];
    out[i] = '\0';
    return i;
}

static int vfmt(struct sink *s, const char *fmt, va_list ap)
{
    for (; *fmt; fmt++) {
        if (*fmt != '%') {
            sink_putc(s, *fmt);
            continue;
        }
        fmt++;

        int left = 0, zero = 0, plus = 0, space = 0, alt = 0;
        for (;; fmt++) {
            if (*fmt == '-') left = 1;
            else if (*fmt == '0') zero = 1;
            else if (*fmt == '+') plus = 1;
            else if (*fmt == ' ') space = 1;
            else if (*fmt == '#') alt = 1;
            else break;
        }

        int width = 0;
        if (*fmt == '*') {
            width = va_arg(ap, int);
            if (width < 0) { left = 1; width = -width; }
            fmt++;
        } else {
            while (isdigit((unsigned char)*fmt))
                width = width * 10 + (*fmt++ - '0');
        }

        int prec = -1;
        if (*fmt == '.') {
            fmt++;
            prec = 0;
            if (*fmt == '*') { prec = va_arg(ap, int); fmt++; }
            else while (isdigit((unsigned char)*fmt)) prec = prec * 10 + (*fmt++ - '0');
            if (prec < 0) prec = -1;
        }

        int lng = 0;
        for (;;) {
            if (*fmt == 'l') { lng++; fmt++; }
            else if (*fmt == 'h' || *fmt == 'z' || *fmt == 'j' || *fmt == 't') { fmt++; }
            else break;
        }
        if (lng > 2) lng = 2;

        char conv = *fmt;
        char numbuf[32];
        const char *prefix = "";
        char sign = 0;

        switch (conv) {
        case 'd':
        case 'i': {
            long long v = lng == 2 ? va_arg(ap, long long)
                        : lng == 1 ? va_arg(ap, long)
                                   : va_arg(ap, int);
            unsigned long long mag = v < 0 ? (unsigned long long)(-(v + 1)) + 1ULL
                                           : (unsigned long long)v;
            if (v < 0) sign = '-';
            else if (plus) sign = '+';
            else if (space) sign = ' ';

            int nl = u_to_str(mag, 10, 0, numbuf);
            int zpad = (prec >= 0 && prec > nl) ? prec - nl : 0;
            int body = nl + zpad + (sign ? 1 : 0);
            int wpad = width > body ? width - body : 0;

            if (!left && !zero) sink_pad(s, ' ', wpad);
            if (sign) sink_putc(s, sign);
            if (!left && zero && prec < 0) sink_pad(s, '0', wpad);
            sink_pad(s, '0', zpad);
            sink_write(s, numbuf, nl);
            if (left) sink_pad(s, ' ', wpad);
            break;
        }
        case 'u':
        case 'o':
        case 'x':
        case 'X': {
            unsigned base = conv == 'o' ? 8 : (conv == 'u' ? 10 : 16);
            unsigned long long v = lng == 2 ? va_arg(ap, unsigned long long)
                                 : lng == 1 ? va_arg(ap, unsigned long)
                                            : va_arg(ap, unsigned int);
            int nl = u_to_str(v, base, conv == 'X', numbuf);
            if (alt && (conv == 'x' || conv == 'X') && v)
                prefix = conv == 'x' ? "0x" : "0X";
            int plen = (int)strlen(prefix);
            int zpad = (prec >= 0 && prec > nl) ? prec - nl : 0;
            int body = nl + zpad + plen;
            int wpad = width > body ? width - body : 0;

            if (!left && !zero) sink_pad(s, ' ', wpad);
            sink_write(s, prefix, plen);
            if (!left && zero && prec < 0) sink_pad(s, '0', wpad);
            sink_pad(s, '0', zpad);
            sink_write(s, numbuf, nl);
            if (left) sink_pad(s, ' ', wpad);
            break;
        }
        case 'c': {
            char c = (char)va_arg(ap, int);
            int wpad = width > 1 ? width - 1 : 0;
            if (!left) sink_pad(s, ' ', wpad);
            sink_putc(s, c);
            if (left) sink_pad(s, ' ', wpad);
            break;
        }
        case 's': {
            const char *str = va_arg(ap, const char *);
            if (!str) str = "(null)";
            int n = (int)strnlen(str, prec >= 0 ? (size_t)prec : (size_t)-1);
            int wpad = width > n ? width - n : 0;
            if (!left) sink_pad(s, ' ', wpad);
            sink_write(s, str, n);
            if (left) sink_pad(s, ' ', wpad);
            break;
        }
        case 'p': {
            void *ptr = va_arg(ap, void *);
            int nl = u_to_str((unsigned long long)(uintptr_t)ptr, 16, 0, numbuf);
            int wpad = width > nl + 2 ? width - nl - 2 : 0;
            if (!left) sink_pad(s, ' ', wpad);
            sink_write(s, "0x", 2);
            sink_write(s, numbuf, nl);
            if (left) sink_pad(s, ' ', wpad);
            break;
        }
        case 'n': {
            int *out = va_arg(ap, int *);
            if (out) *out = (int)s->len;
            break;
        }
        case '%':
            sink_putc(s, '%');
            break;
        case '\0':
            fmt--;
            break;
        default:
            sink_putc(s, '%');
            sink_putc(s, conv);
            break;
        }
    }
    sink_flush(s);
    return (int)s->len;
}

/* shared with stdio_file.c */
int __vfmt_buf(char *buf, size_t n, const char *fmt, va_list ap)
{
    struct sink s = { .buf = buf, .cap = n, .len = 0, .fd = -1 };
    int r = vfmt(&s, fmt, ap);
    if (buf && n)
        buf[s.len < n ? s.len : n - 1] = '\0';
    return r;
}

int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap)
{
    return __vfmt_buf(buf, n, fmt, ap);
}

int vsprintf(char *buf, const char *fmt, va_list ap)
{
    return __vfmt_buf(buf, (size_t)-1, fmt, ap);
}

int snprintf(char *buf, size_t n, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = __vfmt_buf(buf, n, fmt, ap);
    va_end(ap);
    return r;
}

int sprintf(char *buf, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = __vfmt_buf(buf, (size_t)-1, fmt, ap);
    va_end(ap);
    return r;
}

static int vfd(int fd, const char *fmt, va_list ap)
{
    struct sink s = { .buf = NULL, .cap = 0, .len = 0, .fd = fd };
    return vfmt(&s, fmt, ap);
}

int vprintf(const char *fmt, va_list ap)
{
    return vfd(1, fmt, ap);
}

int printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = vfd(1, fmt, ap);
    va_end(ap);
    return r;
}

int putchar(int c)
{
    char ch = (char)c;
    return sys_write(1, &ch, 1) == 1 ? c : EOF;
}

int puts(const char *s)
{
    long n = sys_write(1, s, strlen(s));
    if (n < 0)
        return EOF;
    return putchar('\n') == EOF ? EOF : 0;
}

/* ---- string scanf (fscanf is in stdio_file.c) ----------------------- */

int __vscan_str(const char *str, const char *fmt, va_list ap);

int sscanf(const char *str, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int r = __vscan_str(str, fmt, ap);
    va_end(ap);
    return r;
}
