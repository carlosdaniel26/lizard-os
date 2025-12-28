#include <slab.h>
#include <list.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct CacheNode {
    ListHead list;
    KMemCache *cache;
    size_t size;
} CacheNode;

static LIST_HEAD(cache_list);

void kmalloc_init(void)
{
    InitListHead(&cache_list);

    size_t sizes[] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048};
    char name[KMEMCACHE_NAME_LEN];

    for (size_t i = 0; i < sizeof(sizes)/sizeof(sizes[0]); i++) {
        KMemCache *cache = kmemcache_create("kmalloc_node", sizeof(CacheNode), NULL);
        CacheNode *node = kmemcache_alloc(cache);
        node->size = sizes[i];

        sprintf(name, "kmalloc_%zu", sizes[i]);
        node->cache = kmemcache_create(name, sizes[i], NULL);

        list_add(&node->list, &cache_list);
    }
}

void *kmalloc(size_t size)
{
    ListHead *pos, *tmp;
    CacheNode *best = NULL;

    list_for_each(pos, tmp, &cache_list) {
        CacheNode *node = container_of(pos, CacheNode, list);
        if (node->size >= size && (!best || node->size < best->size))
            best = node;
    }

    if (!best) {
        CacheNode *new_node = kmemcache_alloc(kmemcache_create("kmalloc_node", sizeof(CacheNode), NULL));
        new_node->size = size;
		char name[KMEMCACHE_NAME_LEN];
        sprintf(name, "kmalloc_%u", size);
        new_node->cache = kmemcache_create(name, size, NULL);

        list_add(&new_node->list, &cache_list);
        best = new_node;
    }

    return kmemcache_alloc(best->cache);
}

void kfree(void *ptr)
{
    ListHead *pos, *tmp;
    list_for_each(pos, tmp, &cache_list) {
        CacheNode *node = container_of(pos, CacheNode, list);
        if (kmemcache_free(node->cache, ptr) == 0)
            return;
    }
}

void *zalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void krealloc(void **ptr, size_t old_size, size_t new_size)
{
    if (!ptr || new_size == 0)
        return;

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr)
        return;

    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, *ptr, copy_size);
    kfree(*ptr);
    *ptr = new_ptr;
}

void zrealloc(void **ptr, size_t old_size, size_t new_size)
{
    if (!ptr || new_size == 0)
        return;

    void *new_ptr = kmalloc(new_size);
    if (!new_ptr)
        return;

    size_t copy_size = (old_size < new_size) ? old_size : new_size;
    memcpy(new_ptr, *ptr, copy_size);

    size_t zero_size = (new_size > old_size) ? (new_size - old_size) : 0;
    if (zero_size > 0)
        memset((char *)new_ptr + copy_size, 0, zero_size);

    kfree(*ptr);
    *ptr = new_ptr;
}