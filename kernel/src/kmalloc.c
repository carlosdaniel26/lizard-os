#include <helpers.h>
#include <string.h>
#include <kmalloc.h>
#include <pmm.h>

#define BLOCK_SIZE 4096
#define START_BLOCKS 10
#define START_HEAP_SIZE BLOCK_SIZE * START_BLOCKS

#define DIV_ROUND_UP(x, y) (((x) + (y)-1) / (y))

static KMemoryHeader *ptr_free = NULL;
static KMemoryHeader *ptr_heap_end = NULL; /* points to the last heap block allocated*/

static bool kmalloc_extend_heap(size_t size)
{
    uint64_t blocks = (uint64_t)DIV_ROUND_UP(size, 4096);

    void *new_block_addr = pmm_alloc_block_row(blocks) + hhdm_offset;
    if (!new_block_addr)
        return false;

    KMemoryHeader *new_block = (KMemoryHeader *)new_block_addr;
    new_block->size = BLOCK_SIZE - sizeof(KMemoryHeader);
    new_block->is_free = true;
    new_block->next = NULL;
    new_block->prev = NULL;

    /* Insert new block at the end of free list */
    KMemoryHeader *last = ptr_free;
    if (!last)
    {
        ptr_free = new_block;
        ptr_heap_end = new_block;
        return true;
    }
    while (last->next)
        last = last->next;

    last->next = new_block;
    new_block->prev = last;
    ptr_heap_end = new_block;

    return true;
}

void *kmalloc(size_t n_bytes)
{
    if (!ptr_free)
        kmalloc_extend_heap(n_bytes);

    size_t total_needed = 0;

    for (int attempt = 0; attempt < 2; attempt++)
    {
        KMemoryHeader *current = ptr_free;

        while (current)
        {
            if (!current->is_free || current->size < n_bytes)
            {
                current = current->next;
                continue;
            }

            total_needed = n_bytes + sizeof(KMemoryHeader);

            /*
             * Only split if leftover block after allocation
             * is big enough for header + 16 bytes of data.
             * Otherwise allocate entire block.
             */

            if (current->size >= total_needed + sizeof(KMemoryHeader) + 16)
            {
                KMemoryHeader *new_block =
                    (KMemoryHeader *)((char *)current + sizeof(KMemoryHeader) + n_bytes);
                new_block->size = current->size - n_bytes - sizeof(KMemoryHeader);
                new_block->is_free = true;
                new_block->next = current->next;
                new_block->prev = current;

                if (new_block->next)
                    new_block->next->prev = new_block;

                current->next = new_block;
                current->size = n_bytes;
            }

            current->is_free = false;
            return (void *)(current + sizeof(KMemoryHeader));
        }

        /* No suitable block found, try to extend heap*/
        if (!kmalloc_extend_heap(total_needed))
        {
            /* Out of memory: can't extend heap further*/
            break;
        }
    }

    return NULL;
}

void *kcalloc(size_t n_bytes)
{
    void *ptr = kmalloc(n_bytes);
    memset(ptr, 0, n_bytes);
    
    return ptr;
}

void *krealloc(void *ptr, size_t n_bytes)
{
    if (!ptr)
        return kmalloc(n_bytes);

    KMemoryHeader *block = (KMemoryHeader *)((char *)ptr - sizeof(KMemoryHeader));

    if (block->size >= n_bytes)
        return ptr;

    void *new_ptr = kmalloc(n_bytes);
    if (!new_ptr)
        return NULL;

    memcpy(new_ptr, ptr, block->size);
    kfree(ptr);

    return new_ptr;
}

void kfree(void *ptr)
{
    KMemoryHeader *block = (KMemoryHeader *)((char *)ptr - sizeof(KMemoryHeader));

    block->is_free = true;

    /* Merge with next if free*/
    if (block->next != NULL && block->next->is_free)
    {
        block->size += block->next->size + sizeof(KMemoryHeader);
        block->next = block->next->next;

        if (block->next)
            block->next->prev = block;
    }

    /* Merge with previous if free */
    if (block->prev != NULL && block->prev->is_free)
    {
        block->prev->size += block->size + sizeof(KMemoryHeader);
        block->prev->next = block->next;
        if (block->next)
            block->next->prev = block->prev;

        if (block == ptr_free)
            ptr_free = block->prev;
    }
}