#include <kmalloc.h>
#include <types.h>
#include <string.h>
#include <buddy.h>
#include <pgtable.h>

/* stupid alloc just to start to use the interface */

void *kmalloc(size_t size)
{
    if (size == 0)
        return NULL;

    size = size + sizeof(size_t); /* extra space to store size */
    size_t pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    size_t *ptr = buddy_alloc(PAGES_TO_ORDER(pages));
    *ptr = pages;

    return ptr + sizeof(size_t);
}

void kfree(void *ptr)
{
    if (ptr == NULL)
        return;
    size_t size = *((size_t *)ptr - sizeof(size_t));
    ptr = (size_t *)ptr - sizeof(size_t);

    buddy_free(ptr, size);
}

void *kcalloc(size_t size)
{
    void *ptr = kmalloc(size);
    if (ptr)
    {
        memset(ptr, 0, size);
    }
    return ptr;
}