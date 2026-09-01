/* Minimal *scanf core. Covers what doom's m_misc.c / m_config.c actually use:
 * %d %i %u %x %o %s %c %[...] with optional width and '*' suppression. */
#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include "libc_internal.h"

int __vscan_core(struct src *sc, const char *fmt, va_list ap)
{
    int assigned = 0;

    for (; *fmt; fmt++) {
        if (isspace((unsigned char)*fmt)) {
            int c;
            do { c = sc->get(sc->ctx); } while (isspace(c));
            sc->unget(sc->ctx, c);
            continue;
        }
        if (*fmt != '%') {
            int c = sc->get(sc->ctx);
            if (c != *fmt) {
                sc->unget(sc->ctx, c);
                return assigned ? assigned : (c == EOF ? EOF : 0);
            }
            continue;
        }

        fmt++;
        int suppress = 0;
        if (*fmt == '*') { suppress = 1; fmt++; }

        long width = 0;
        while (isdigit((unsigned char)*fmt))
            width = width * 10 + (*fmt++ - '0');
        if (width == 0)
            width = (1L << 40);

        while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z')
            fmt++;

        char conv = *fmt;
        int c;

        if (conv == 'd' || conv == 'i' || conv == 'u' || conv == 'x' || conv == 'o') {
            int base = conv == 'x' ? 16 : (conv == 'o' ? 8 : 10);
            do { c = sc->get(sc->ctx); } while (isspace(c));

            int neg = 0;
            if (c == '+' || c == '-') { neg = (c == '-'); c = sc->get(sc->ctx); width--; }

            if ((conv == 'i' || conv == 'x') && c == '0') {
                int c2 = sc->get(sc->ctx);
                if (c2 == 'x' || c2 == 'X') { base = 16; c = sc->get(sc->ctx); width -= 2; }
                else { sc->unget(sc->ctx, c2); if (conv == 'i') base = 8; }
            }

            long val = 0;
            int any = 0;
            while (width-- > 0) {
                int d;
                if (c >= '0' && c <= '9') d = c - '0';
                else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') d = c - 'A' + 10;
                else break;
                if (d >= base) break;
                val = val * base + d;
                any = 1;
                c = sc->get(sc->ctx);
            }
            sc->unget(sc->ctx, c);
            if (!any)
                return assigned ? assigned : EOF;
            if (!suppress) {
                *va_arg(ap, int *) = (int)(neg ? -val : val);
                assigned++;
            }
        } else if (conv == 's') {
            do { c = sc->get(sc->ctx); } while (isspace(c));
            char *out = suppress ? 0 : va_arg(ap, char *);
            int n = 0;
            while (c != EOF && !isspace(c) && width-- > 0) {
                if (out) out[n] = (char)c;
                n++;
                c = sc->get(sc->ctx);
            }
            sc->unget(sc->ctx, c);
            if (n == 0)
                return assigned ? assigned : EOF;
            if (out) out[n] = '\0';
            if (!suppress) assigned++;
        } else if (conv == 'c') {
            char *out = suppress ? 0 : va_arg(ap, char *);
            if (width == (1L << 40)) width = 1;
            int n = 0;
            while (width-- > 0) {
                c = sc->get(sc->ctx);
                if (c == EOF) break;
                if (out) out[n] = (char)c;
                n++;
            }
            if (n == 0)
                return assigned ? assigned : EOF;
            if (!suppress) assigned++;
        } else if (conv == '[') {
            fmt++;
            int negate = 0;
            if (*fmt == '^') { negate = 1; fmt++; }
            const char *set = fmt;
            while (*fmt && *fmt != ']')
                fmt++;
            long setlen = (long)(fmt - set);

            char *out = suppress ? 0 : va_arg(ap, char *);
            int n = 0;
            c = sc->get(sc->ctx);
            while (c != EOF && width-- > 0) {
                int in = 0;
                for (long i = 0; i < setlen; i++)
                    if ((unsigned char)set[i] == c) { in = 1; break; }
                if (in == negate)
                    break;
                if (out) out[n] = (char)c;
                n++;
                c = sc->get(sc->ctx);
            }
            sc->unget(sc->ctx, c);
            if (out) out[n] = '\0';
            if (n == 0)
                return assigned ? assigned : EOF;
            if (!suppress) assigned++;
        } else if (conv == '%') {
            do { c = sc->get(sc->ctx); } while (isspace(c));
            if (c != '%') { sc->unget(sc->ctx, c); return assigned; }
        } else {
            return assigned;
        }
    }
    return assigned;
}

/* ---- string source ------------------------------------------------- */

struct strctx {
    const char *p;
    int pushed;
};

static int str_get(void *v)
{
    struct strctx *s = v;
    if (s->pushed >= 0) { int c = s->pushed; s->pushed = -1; return c; }
    return *s->p ? (unsigned char)*s->p++ : EOF;
}

static void str_unget(void *v, int c)
{
    struct strctx *s = v;
    if (c != EOF)
        s->pushed = c;
}

int __vscan_str(const char *str, const char *fmt, va_list ap)
{
    struct strctx ctx = { .p = str, .pushed = -1 };
    struct src sc = { .get = str_get, .unget = str_unget, .ctx = &ctx };
    return __vscan_core(&sc, fmt, ap);
}
