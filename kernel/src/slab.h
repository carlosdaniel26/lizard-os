#pragma once

#include <list.h>
#include <spinlock.h>
#include <stddef.h>

#define KMEMCACHE_NAME_LEN 32
#define SLAB_MAGIC 0xDEADBEEF

typedef struct KMemCache {
    char name[KMEMCACHE_NAME_LEN];

    size_t object_size;           /* requested size */
    size_t real_object_size;      /* actual size including metadata */
    size_t align;                 /* object alignment */
    size_t size;                  /* actual size including metadata */
    unsigned int in_use;          /* total allocated objects */
    unsigned int free_slab_count; /* number of free slabs */

    unsigned int objects_per_slab;

    unsigned int order; /* pages per slab = 2^order */

    ListHead slabs_full;
    ListHead slabs_partial;
    ListHead slabs_free;

    unsigned long flags;

    void (*ctor)(void *obj);
    void (*dtor)(void *obj);

    spinlock_t lock;
} KMemCache;

typedef struct Slab {
    ListHead list;

    void *start;         /* base address of slab */
    unsigned int in_use; /* allocated objects */
    unsigned int free;   /* free objects */

    void *freelist; /* singly-linked list of free objs */
    unsigned long magic;

    KMemCache *cache;
} Slab;

typedef struct SlabObj {
    struct Slab *slab;    /* owning slab */
    struct SlabObj *next; /* freelist linkage */
} SlabObj;

KMemCache *kmemcache_create(const char *name, size_t obj_size, void (*ctor)(void *), void (*dtor)(void *));
int kmemcache_destroy(KMemCache *cache);

void *kmemcache_alloc(KMemCache *cache);
int kmemcache_free(void *obj);

Slab *_get_slab(void *obj);
int _is_valid_slab(Slab *slab);