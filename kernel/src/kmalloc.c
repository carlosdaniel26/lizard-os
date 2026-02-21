#include <slab.h>
#include <list.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <types.h>
#include <pgtable.h>
#include <helpers.h>
#include <kmalloc.h>
#include <early_alloc.h>
#include <buddy.h>
#include <debug.h>
#include <panic.h>

#define DIV_ROUND_UP(x, y) (((x) + (y)-1) / (y))
#define BUDDYMALLOC_MAGIC 0x424D414C  /* "BMAL" */

typedef struct CacheNode {
    ListHead list;
    KMemCache *cache;
    size_t size;
} CacheNode;

typedef struct BuddyKmallocHeader {
    uint32_t magic; /* BUDDYMALLOC_MAGIC */
    uint8_t order;
    uint8_t padding[3];
} BuddyKmallocHeader;

static LIST_HEAD(kmalloc_cache_list);
static KMemCache *kmalloc_node_cache = NULL;

void kmalloc_init(void)
{
    InitListHead(&kmalloc_cache_list);
    kmalloc_node_cache = kmemcache_create("kmalloc_node", sizeof(CacheNode), NULL, NULL);

    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048}; // bytes
    char name[KMEMCACHE_NAME_LEN];

    for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        CacheNode *node = kmemcache_alloc(kmalloc_node_cache);
        node->size = sizes[i];

        sprintf(name, "kmalloc_%u", (unsigned)sizes[i]);
        node->cache = kmemcache_create(name, sizes[i], NULL, NULL);

        list_add(&node->list, &kmalloc_cache_list);
    }
}

void *kmalloc(size_t size)
{   
    ListHead *pos, *tmp;
    CacheNode *best = NULL;

    list_for_each(pos, tmp, &kmalloc_cache_list) {
        CacheNode *node = container_of(pos, CacheNode, list);
        if (node->size >= size && (!best || node->size < best->size))
            best = node;
    }

    if (! best)
    {
        u64 required_space = sizeof(BuddyKmallocHeader) + size;
        unsigned pg_count = DIV_ROUND_UP(required_space, PAGE_SIZE);
        int order = pages_to_order(pg_count);

        BuddyKmallocHeader *header = buddy_alloc(order);
        if (!header) {
            debug_printf("kmalloc: buddy_alloc failed for size %llu\n", required_space);
            return NULL;
        }

        header->magic = BUDDYMALLOC_MAGIC;
        header->order = order;

        debug_printf("kmalloc %p order %u\n", header, order);

        return (void*)(header + 1);
    }

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
    else /* no, it came from buddy */
    {
        BuddyKmallocHeader *header = (BuddyKmallocHeader *)ptr - 1;

        if (header->magic != BUDDYMALLOC_MAGIC) {
            kpanic("kfree: BuddyKmallocHeader magic mismatch %p", ptr);
        }

        unsigned int order = header->order;
        debug_printf("kfree %p block %p order %u\n", ptr, header, order);
        buddy_free(header, order);
    }
}

void *zalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}
