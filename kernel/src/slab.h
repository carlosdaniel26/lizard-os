#pragma once

#include <list.h>
#include <spinlock.h>


#define KMEMCACHE_NAME_LEN 32
#define SLAB_MAGIC 0xDEADBEEF

struct kmem_cache {
    char name[KMEMCACHE_NAME_LEN];

    size_t object_size;           /* requested size */
    size_t real_object_size;      /* actual size including metadata */
    size_t align;                 /* object alignment */
    size_t size;                  /* actual size including metadata */
    unsigned int in_use;          /* total allocated objects */
    unsigned int free_slab_count; /* number of free slabs */

    unsigned int objects_per_slab;

    unsigned int order; /* pages per slab = 2^order */

    struct list_head slabs_full;
    struct list_head slabs_partial;
    struct list_head slabs_free;

    unsigned long flags;

    void (*ctor)(void *obj);
    void (*dtor)(void *obj);

    struct spinlock_t lock;
};

struct slab {
    struct list_head list;

    void *start;         /* base address of slab */
    unsigned int in_use; /* allocated objects */
    unsigned int free;   /* free objects */

    void *freelist; /* singly-linked list of free objs */
    unsigned long magic;

    struct kmem_cache *cache;
};

struct slab_obj {
    struct slab *slab;    /* owning slab */
    struct slab_obj *next; /* freelist linkage */
};

struct kmem_cache *kmemcache_create(const char *name, size_t obj_size, void (*ctor)(void *), void (*dtor)(void *));
int kmemcache_destroy(struct kmem_cache *cache);

void *kmemcache_alloc(struct kmem_cache *cache);
int kmemcache_free(void *obj);

struct slab *_get_slab(void *obj);
int _is_valid_slab(struct slab *slab);