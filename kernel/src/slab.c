#include <slab.h>
#include <pgtable.h>
#include <buddy.h>
#include <string.h>
#include <helpers.h>
#include <stdio.h>
#include <list.h>

#define MAX_FREE_SLABS 3
#define MAX_OBJS_PER_SLAB 1024

KMemCache *kmemcache_create(const char *name, size_t obj_size, void *ctor)
{
    size_t required_bytes = sizeof(Slab) + obj_size * MAX_OBJS_PER_SLAB;
    int order = 0;

    while ((size_t)PAGE_SIZE << order < required_bytes)
        order++;

    KMemCache *cache = buddy_alloc(0);
    if (!cache)
        return NULL;

    cache->object_size = obj_size;
    cache->order = order;
    cache->free_slab_count = 0;
    cache->in_use = 0;

    size_t slabs_bytes = (size_t)PAGE_SIZE << order;
    cache->objects_per_slab = (slabs_bytes - sizeof(Slab)) / obj_size;
    if (cache->objects_per_slab == 0)
    {
        debug_printf("on %s", name);
        kpanic("kmemcache_create: object size too big for slab\n");
    }

    if (cache->objects_per_slab > MAX_OBJS_PER_SLAB)
    {
        cache->objects_per_slab = MAX_OBJS_PER_SLAB;
    }

    cache->ctor = ctor;

    cache->slabs_free.next = cache->slabs_free.prev = &cache->slabs_free;
    cache->slabs_partial.next = cache->slabs_partial.prev = &cache->slabs_partial;
    cache->slabs_full.next = cache->slabs_full.prev = &cache->slabs_full;

    if (name)
    {
        strncpy(cache->name, name, KMEMCACHE_NAME_LEN - 1);
        cache->name[KMEMCACHE_NAME_LEN - 1] = '\0';
    }
    else
    {
        cache->name[0] = '\0';
    }

    return cache;
}

int kmemcache_destroy(KMemCache *cache)
{
    if (cache->in_use > 0)
        return -1;

    if (cache->slabs_full.next != &cache->slabs_full)
        return -1;

    if (cache->slabs_free.next != &cache->slabs_free)
    {
        ListHead *pos, *tmp;
        list_for_each(pos, tmp, &cache->slabs_free)
        {
            Slab *slab = container_of(pos, Slab, list);
            slab_destroy(cache, slab);
        }
    }

    buddy_free(cache, 0);
    return 0;
}

Slab *slab_create(KMemCache *cache)
{
    Slab *slab = buddy_alloc(cache->order);
    if (!slab)
        return NULL;

    slab->cache = cache;
    slab->start = (void *)slab + sizeof(Slab);
    slab->in_use = 0;
    slab->free = cache->objects_per_slab;
    slab->freelist = slab->start;

    uintptr_t obj_addr = (uintptr_t)slab->start;

    for (unsigned int i = 0; i < cache->objects_per_slab - 1; i++)
    {
        uintptr_t next_obj_addr = obj_addr + cache->object_size;
        *(void **)obj_addr = (void *)next_obj_addr;
        obj_addr = next_obj_addr;
    }

    *(void **)obj_addr = NULL;

    list_add(&slab->list, &cache->slabs_free);

    return slab;
}

void slab_destroy(KMemCache *cache, Slab *slab)
{
    list_del(&slab->list);
    buddy_free(slab, cache->order);
}

void *kmemcache_alloc(KMemCache *cache)
{
    Slab *slab;

    if (cache->slabs_partial.next != &cache->slabs_partial)
    {
        slab = list_first_entry(&cache->slabs_partial, Slab, list);
    }
    else if (cache->slabs_free.next != &cache->slabs_free)
    {
        slab = list_first_entry(&cache->slabs_free, Slab, list);
        list_move(&slab->list, &cache->slabs_partial);
        cache->free_slab_count--;
    }
    else
    {
        slab = slab_create(cache);
        if (!slab)
            return NULL;

        list_move(&slab->list, &cache->slabs_partial);
    }

    void *obj = slab->freelist;
    slab->freelist = *(void **)obj;

    slab->cache->in_use++;
    slab->in_use++;
    slab->free--;

    if (slab->in_use == cache->objects_per_slab)
        list_move(&slab->list, &cache->slabs_full);

    if (cache->ctor)
        cache->ctor(obj);

    return obj;
}

int kmemcache_free(KMemCache *cache, void *obj)
{
    Slab *slab = align_ptr_down(obj, (size_t)(PAGE_SIZE << cache->order));

    if ((uintptr_t)obj < (uintptr_t)slab->start ||
        (uintptr_t)obj >= (uintptr_t)slab->start + slab->cache->objects_per_slab * slab->cache->object_size)
    {
        return -1;
    }

    int was_full = (slab->in_use == cache->objects_per_slab);

    slab->cache->in_use--;
    slab->in_use--;
    slab->free++;

    if (was_full)
        list_move(&slab->list, &cache->slabs_partial);

    if (slab->in_use == 0)
    {
        int free_slabs = cache->free_slab_count + 1;

        if (free_slabs >= MAX_FREE_SLABS)
        {
            slab_destroy(cache, slab);
        }
        else
        {
            list_move(&slab->list, &cache->slabs_free);
            cache->free_slab_count++;
        }
    }

    if (cache->dtor)
        cache->dtor(obj);

    *(void **)obj = slab->freelist;
    slab->freelist = obj;

    return 0;
}
