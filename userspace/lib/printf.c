#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <stdarg.h>

/* ---- minimal printf --------------------------------------------------- */

#define PRINTF_BUF 256

static char *put_str(char *p, char *end, const char *s)
{
    while (*s && p < end)
        *p++ = *s++;
    return p;
}

/* base 10 or 16; `sgn` non-zero treats val as signed */
static char *put_num(char *p, char *end, unsigned long val, unsigned base, int sgn)
{
    char tmp[24];
    int i = 0;
    int neg = 0;

    if (sgn && (long)val < 0) {
        neg = 1;
        val = (unsigned long)(-(long)val);
    }
    do {
        unsigned d = (unsigned)(val % base);
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        val /= base;
    } while (val);
    if (neg) tmp[i++] = '-';

    while (i-- > 0 && p < end)
        *p++ = tmp[i];
    return p;
}

int printf(const char *fmt, ...)
{
    char buf[PRINTF_BUF];
    char *p = buf;
    char *end = buf + sizeof(buf);
    va_list ap;

    va_start(ap, fmt);
    for (; *fmt && p < end; fmt++) {
        if (*fmt != '%') {
            *p++ = *fmt;
            continue;
        }
        switch (*++fmt) {
        case 's': p = put_str(p, end, va_arg(ap, const char *));       break;
        case 'c': if (p < end) *p++ = (char)va_arg(ap, int);           break;
        case 'd':
        case 'i': p = put_num(p, end, (unsigned long)(long)va_arg(ap, int), 10, 1); break;
        case 'u': p = put_num(p, end, va_arg(ap, unsigned), 10, 0);    break;
        case 'x': p = put_num(p, end, va_arg(ap, unsigned), 16, 0);    break;
        case 'p':
            p = put_str(p, end, "0x");
            p = put_num(p, end, (unsigned long)va_arg(ap, void *), 16, 0);
            break;
        case '%': if (p < end) *p++ = '%';                             break;
        case '\0': fmt--;                                              break;
        default:  if (p < end) *p++ = *fmt;                            break;
        }
    }
    va_end(ap);

    long n = sys_write(1, buf, (size_t)(p - buf));
    return n < 0 ? -1 : (int)n;
}
