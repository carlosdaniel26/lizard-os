#include <buddy.h>
#include <debug.h>
#include <helpers.h>
#include <list.h>
#include <panic.h>
#include <pgtable.h>
#include <slab.h>
#include <stdio.h>
#include <string.h>

#define MAX_FREE_SLABS 3

/* ============================================================
 * struct slab object header
 * ============================================================ */

/* ============================================================
 * struct slab helpers
 * ============================================================ */

static struct slab *slab_create(struct kmem_cache *cache)
{
    void *mem = buddy_alloc(cache->order);
    if (!mem) return NULL;

    size_t slab_size = (size_t)PAGE_SIZE << cache->order;
    uintptr_t slab_start = (uintptr_t)mem;
    uintptr_t slab_end = slab_start + slab_size;

    struct slab *slab = (struct slab *)mem;
    slab->magic = SLAB_MAGIC;
    slab->cache = cache;
    slab->start = mem;
    slab->in_use = 0;
    slab->freelist = NULL;

    slab->list.next = slab->list.prev = &slab->list;

    uintptr_t obj_start = align_up(slab_start + sizeof(struct slab), cache->align);

    if (obj_start >= slab_end) kpanic("slab_create: no space for objects");

    size_t usable = slab_end - obj_start;
    unsigned int objs = usable / cache->real_object_size;

    if (objs == 0) kpanic("slab_create: zero objects");

    slab->free = objs;

    for (int i = objs - 1; i >= 0; i--)
    {
        struct slab_obj *o = (struct slab_obj *)(obj_start + i * cache->real_object_size);

        o->slab = slab;
        o->next = slab->freelist;
        slab->freelist = o;
    }

    return slab;
}

static void slab_destroy(struct kmem_cache *cache, struct slab *slab)
{
    if (slab->magic != SLAB_MAGIC) kpanic("slab_destroy: bad slab");

    list_del(&slab->list);
    slab->magic = 0;

    buddy_free(slab, cache->order);
}

/* ============================================================
 * Cache API
 * ============================================================ */

struct kmem_cache *kmemcache_create(const char *name, size_t obj_size, void (*ctor)(void *), void (*dtor)(void *))
{
    struct kmem_cache *cache = buddy_alloc(0);
    if (!cache) return NULL;

    memset(cache, 0, sizeof(*cache));

    if (name)
    {
        strncpy(cache->name, name, KMEMCACHE_NAME_LEN - 1);
        cache->name[KMEMCACHE_NAME_LEN - 1] = 0;
    }

    cache->object_size = obj_size;
    cache->align = sizeof(void *);

    cache->real_object_size = align_up(sizeof(struct slab_obj) + obj_size, cache->align);

    size_t needed = sizeof(struct slab) + cache->real_object_size;

    cache->order = 0;
    while (((size_t)PAGE_SIZE << cache->order) < needed)
        cache->order++;

    size_t slab_bytes = (size_t)PAGE_SIZE << cache->order;

    cache->objects_per_slab = (slab_bytes - sizeof(struct slab)) / cache->real_object_size;

    if (cache->objects_per_slab == 0) kpanic("kmemcache_create: object too large");

    cache->ctor = ctor;
    cache->dtor = dtor;

    // INIT_LIST_HEAD(&cache->slabs_full);
    // INIT_LIST_HEAD(&cache->slabs_partial);
    // INIT_LIST_HEAD(&cache->slabs_free);
    cache->slabs_full.next = cache->slabs_full.prev = &cache->slabs_full;
    cache->slabs_partial.next = cache->slabs_partial.prev = &cache->slabs_partial;
    cache->slabs_free.next = cache->slabs_free.prev = &cache->slabs_free;

    return cache;
}

/* ============================================================
 * Allocation
 * ============================================================ */

void *kmemcache_alloc(struct kmem_cache *cache)
{
    struct slab *slab;

    if (!list_empty(&cache->slabs_partial))
    {
        slab = list_first_entry(&cache->slabs_partial, struct slab, list);
    }
    else if (!list_empty(&cache->slabs_free))
    {
        slab = list_first_entry(&cache->slabs_free, struct slab, list);
        list_move(&slab->list, &cache->slabs_partial);
        cache->free_slab_count--;
    }
    else
    {
        slab = slab_create(cache);
        if (!slab) return NULL;
        list_add(&slab->list, &cache->slabs_partial);
    }

    struct slab_obj *o = slab->freelist;
    slab->freelist = o->next;

    slab->in_use++;
    slab->free--;
    cache->in_use++;

    if (slab->free == 0) list_move(&slab->list, &cache->slabs_full);

    void *obj = (void *)(o + 1);

    if (cache->ctor) cache->ctor(obj);

    return obj;
}

/* ============================================================
 * Free
 * ============================================================ */

int kmemcache_free(void *obj)
{
    if (!obj) return -1;

    struct slab *slab = _get_slab(obj);
    struct kmem_cache *cache = slab->cache;

    if (slab->magic != SLAB_MAGIC) kpanic("kmemcache_free: bad slab %p", slab);

    if (cache->dtor) cache->dtor(obj);

    struct slab_obj *o = ((struct slab_obj *)obj) - 1;
    o->next = slab->freelist;
    slab->freelist = o;

    slab->in_use--;
    slab->free++;
    cache->in_use--;

    if (slab->free == 1) list_move(&slab->list, &cache->slabs_partial);

    if (slab->in_use == 0)
    {
        if (cache->free_slab_count >= MAX_FREE_SLABS)
        {
            slab_destroy(cache, slab);
        }
        else
        {
            list_move(&slab->list, &cache->slabs_free);
            cache->free_slab_count++;
        }
    }

    return 0;
}

/* public helpers */
struct slab *_get_slab(void *obj)
{
    struct slab_obj *o = ((struct slab_obj *)obj) - 1;
    struct slab *slab = o->slab;

    return slab;
}

int _is_valid_slab(struct slab *slab)
{
    return (slab->magic == SLAB_MAGIC);
}