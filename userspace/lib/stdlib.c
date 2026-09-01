#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>

/* ---- heap -----------------------------------------------------------------
 * doom's zone allocator asks for one ~6 MiB block up front (I_ZoneBase) and
 * doomgeneric mallocs the 1 MiB screen buffer; everything else is carved out
 * of the zone. A 16 MiB static arena with a first-fit free list covers it.
 * The arena lands in .bss, so the ELF loader maps it at exec time.
 */
#define HEAP_SIZE (16 * 1024 * 1024)
static unsigned char g_heap[HEAP_SIZE] __attribute__((aligned(16)));

typedef struct block {
    size_t size;         /* payload bytes */
    struct block *next;   /* next block in address order */
    int free;
} block_t;

#define ALIGN_UP(n, a) (((n) + (a) - 1) & ~((size_t)(a) - 1))
#define HDR ALIGN_UP(sizeof(block_t), 16)

static block_t *g_first;

static void heap_init(void)
{
    g_first = (block_t *)g_heap;
    g_first->size = HEAP_SIZE - HDR;
    g_first->next = NULL;
    g_first->free = 1;
}

void *malloc(size_t want)
{
    if (want == 0)
        want = 1;
    want = ALIGN_UP(want, 16);

    if (!g_first)
        heap_init();

    for (block_t *b = g_first; b; b = b->next) {
        if (!b->free || b->size < want)
            continue;

        /* split if the leftover can hold a header + a little payload */
        if (b->size >= want + HDR + 16) {
            block_t *nb = (block_t *)((unsigned char *)b + HDR + want);
            nb->size = b->size - want - HDR;
            nb->next = b->next;
            nb->free = 1;
            b->size = want;
            b->next = nb;
        }
        b->free = 0;
        return (unsigned char *)b + HDR;
    }
    return NULL;
}

static block_t *block_of(void *ptr)
{
    return (block_t *)((unsigned char *)ptr - HDR);
}

void free(void *ptr)
{
    if (!ptr)
        return;

    block_t *b = block_of(ptr);
    b->free = 1;

    /* coalesce forward */
    while (b->next && b->next->free) {
        b->size += HDR + b->next->size;
        b->next = b->next->next;
    }
    /* coalesce backward */
    if (b != g_first) {
        block_t *p = g_first;
        while (p && p->next != b)
            p = p->next;
        if (p && p->free) {
            p->size += HDR + b->size;
            p->next = b->next;
        }
    }
}

void *calloc(size_t n, size_t size)
{
    size_t total = n * size;
    if (size && total / size != n)
        return NULL;
    void *p = malloc(total);
    if (p)
        memset(p, 0, total);
    return p;
}

void *realloc(void *ptr, size_t size)
{
    if (!ptr)
        return malloc(size);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    block_t *b = block_of(ptr);
    if (b->size >= ALIGN_UP(size, 16))
        return ptr;

    void *np = malloc(size);
    if (!np)
        return NULL;
    memcpy(np, ptr, b->size);
    free(ptr);
    return np;
}

/* ---- process exit -------------------------------------------------------- */

#define ATEXIT_MAX 32
static void (*g_atexit[ATEXIT_MAX])(void);
static int g_atexit_n;

int atexit(void (*fn)(void))
{
    if (g_atexit_n >= ATEXIT_MAX)
        return -1;
    g_atexit[g_atexit_n++] = fn;
    return 0;
}

void exit(int code)
{
    for (int i = g_atexit_n - 1; i >= 0; i--)
        g_atexit[i]();
    _exit(code);
}

void abort(void)
{
    _exit(134);
}

/* ---- number parsing --------------------------------------------------- */

static int digitval(int c, int base)
{
    int v;
    if (c >= '0' && c <= '9')
        v = c - '0';
    else if (c >= 'a' && c <= 'z')
        v = c - 'a' + 10;
    else if (c >= 'A' && c <= 'Z')
        v = c - 'A' + 10;
    else
        return -1;
    return v < base ? v : -1;
}

long strtol(const char *s, char **end, int base)
{
    const char *p = s;
    while (*p == ' ' || (*p >= '\t' && *p <= '\r'))
        p++;

    int neg = 0;
    if (*p == '+' || *p == '-')
        neg = (*p++ == '-');

    if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        p += 2;
        base = 16;
    } else if (base == 0 && p[0] == '0') {
        base = 8;
    } else if (base == 0) {
        base = 10;
    }

    long acc = 0;
    int any = 0;
    for (;;) {
        int v = digitval((unsigned char)*p, base);
        if (v < 0)
            break;
        acc = acc * base + v;
        any = 1;
        p++;
    }

    if (end)
        *end = (char *)(any ? p : s);
    return neg ? -acc : acc;
}

unsigned long strtoul(const char *s, char **end, int base)
{
    return (unsigned long)strtol(s, end, base);
}

int atoi(const char *s)
{
    return (int)strtol(s, NULL, 10);
}

long atol(const char *s)
{
    return strtol(s, NULL, 10);
}

double atof(const char *s)
{
    while (*s == ' ' || (*s >= '\t' && *s <= '\r'))
        s++;

    int neg = 0;
    if (*s == '+' || *s == '-')
        neg = (*s++ == '-');

    double v = 0.0;
    while (*s >= '0' && *s <= '9')
        v = v * 10.0 + (*s++ - '0');

    if (*s == '.') {
        s++;
        double scale = 0.1;
        while (*s >= '0' && *s <= '9') {
            v += (*s++ - '0') * scale;
            scale *= 0.1;
        }
    }
    return neg ? -v : v;
}

int abs(int x)
{
    return x < 0 ? -x : x;
}

long labs(long x)
{
    return x < 0 ? -x : x;
}

/* ---- qsort (insertion sort; doom's arrays are tiny) ------------------- */

void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *))
{
    unsigned char *a = base;
    unsigned char tmp[256];
    if (size > sizeof(tmp))
        return; /* doom never sorts anything this wide */

    for (size_t i = 1; i < n; i++) {
        memcpy(tmp, a + i * size, size);
        size_t j = i;
        while (j > 0 && cmp(a + (j - 1) * size, tmp) > 0) {
            memcpy(a + j * size, a + (j - 1) * size, size);
            j--;
        }
        memcpy(a + j * size, tmp, size);
    }
}

/* ---- rng ------------------------------------------------------------- */

static unsigned long g_rand = 1;

int rand(void)
{
    g_rand = g_rand * 1103515245 + 12345;
    return (int)((g_rand >> 16) & RAND_MAX);
}

void srand(unsigned seed)
{
    g_rand = seed;
}

/* ---- stubs --------------------------------------------------------- */

char *getenv(const char *name)
{
    (void)name;
    return NULL;
}

int system(const char *cmd)
{
    (void)cmd;
    return -1;
}
