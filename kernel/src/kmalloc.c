#include <buddy.h>
#include <debug.h>
#include <early_alloc.h>
#include <helpers.h>
#include <kmalloc.h>
#include <list.h>
#include <panic.h>
#include <pgtable.h>
#include <slab.h>

#include <stdio.h>
#include <string.h>
#include <types.h>

#define DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))

#define KMALLOC_SIZES(X)                                                                                     \
    X(8)                                                                                                     \
    X(16)                                                                                                    \
    X(32)                                                                                                    \
    X(64)                                                                                                    \
    X(128)                                                                                                   \
    X(256)                                                                                                   \
    X(512)                                                                                                   \
    X(1024)                                                                                                  \
    X(2048)                                                                                                  \
    X(4096)                                                                                                  \
    X(8192)                                                                                                  \
    X(16384)                                                                                                 \
    X(32768)                                                                                                 \
    X(65536)                                                                                                 \
    X(131072)

typedef struct CacheNode {
    ListHead list;
    KMemCache *cache;
    size_t size;
} CacheNode;

static LIST_HEAD(kmalloc_cache_list);
static KMemCache *kmalloc_node_cache = NULL;

void kmalloc_init(void)
{
    InitListHead(&kmalloc_cache_list);

    kmalloc_node_cache = kmemcache_create("kmalloc_node", sizeof(CacheNode), NULL, NULL);

    char name[KMEMCACHE_NAME_LEN];

#define CREATE_CACHE(sz)                                                                                     \
    do                                                                                                       \
    {                                                                                                        \
        CacheNode *node = kmemcache_alloc(kmalloc_node_cache);                                               \
        node->size = (sz);                                                                                   \
        sprintf(name, "kmalloc_%u", (unsigned)(sz));                                                         \
        node->cache = kmemcache_create(name, (sz), NULL, NULL);                                              \
        list_add(&node->list, &kmalloc_cache_list);                                                          \
    }                                                                                                        \
    while (0);

    KMALLOC_SIZES(CREATE_CACHE);

#undef CREATE_CACHE
}

void *kmalloc(size_t size)
{
    ListHead *pos, *tmp;
    CacheNode *best = NULL;

    list_for_each(pos, tmp, &kmalloc_cache_list)
    {
        CacheNode *node = container_of(pos, CacheNode, list);
        if (node->size >= size && (!best || node->size < best->size)) best = node;
    }

    if (!best) kpanic("kmalloc: no suitable cache for size %u", (unsigned)size);

    void *addr = kmemcache_alloc(best->cache);

    return addr;
}

void kfree(void *ptr)
{
    debug_printf("kfree %p\n", ptr);
    /* belongs to a slab? */
    Slab *slab = _get_slab(ptr);
    if (_is_valid_slab(slab))
    {
        kmemcache_free(ptr);
    }
    else
    {
        kpanic("kfree: invalid pointer %p", ptr);
    }
}

void *zalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (ptr) memset(ptr, 0, size);
    return ptr;
}
