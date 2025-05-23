#include <kernel/mem/kmalloc.h>
#include <kernel/mem/pmm.h>
#include <kernel/utils/helpers.h>

#define BLOCK_SIZE 4096
#define HEAP_SIZE_MB 16
#define HEAP_TOTAL_SIZE (HEAP_SIZE_MB * 1024 * 1024)
#define HEAP_BLOCKS (HEAP_TOTAL_SIZE / BLOCK_SIZE)

extern uint32_t mem_ammount_b;

static KMemoryHeader* ptr_free = NULL;
static KMemoryHeader* ptr_heap_end = NULL; /* points to the last heap block allocated*/

static inline void kmalloc_init()
{
	void* heap_base = pmm_alloc_block(1);
	if (! heap_base) return;

	/* Allocate initial 16 MB (HEAP_BLOCKS blocks) */
	for (size_t i = 1; i < HEAP_BLOCKS; i++) {
		pmm_alloc_block(1);
	}

	ptr_free = (KMemoryHeader*)heap_base;
	ptr_free->size = HEAP_TOTAL_SIZE - sizeof(KMemoryHeader);
	ptr_free->next = NULL;
	ptr_free->prev = NULL;
	ptr_free->is_free = true;

	ptr_heap_end = (KMemoryHeader*)((char*)heap_base + HEAP_TOTAL_SIZE - sizeof(KMemoryHeader));
}

static bool kmalloc_extend_heap()
{
	void* new_block_addr = pmm_alloc_block(1);
	if (! new_block_addr)
		return false;

	KMemoryHeader* new_block = (KMemoryHeader*)new_block_addr;
	new_block->size = BLOCK_SIZE - sizeof(KMemoryHeader);
	new_block->is_free = true;
	new_block->next = NULL;
	new_block->prev = NULL;

	/* Insert new block at the end of free list */
	KMemoryHeader* last = ptr_free;
	if (!last) {
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

void* kmalloc(size_t n_bytes)
{
	if (! ptr_free)
		kmalloc_init();

	while (true)
	{
		KMemoryHeader* current = ptr_free;

		while (current)
		{
			if (! current->is_free || current->size < n_bytes) {
				current = current->next;
				continue;
			}

			size_t total_needed = n_bytes + sizeof(KMemoryHeader);

			/* 
			 * Only split if leftover block after allocation
			 * is big enough for header + 16 bytes of data.
			 * Otherwise allocate entire block.
			 */

			if (current->size >= total_needed + sizeof(KMemoryHeader) + 16)
			{
				KMemoryHeader* new_block = (KMemoryHeader*)((char*)current + sizeof(KMemoryHeader) + n_bytes);
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
			return (void*)((char*)current + sizeof(KMemoryHeader));
		}

		/* No suitable block found, try to extend heap*/
		if (! kmalloc_extend_heap()) {
			/* Out of memory: can't extend heap further*/
			return NULL;
		}
	}
}

void kfree(void* ptr)
{
	KMemoryHeader *block = (KMemoryHeader*)((char*)ptr - sizeof(KMemoryHeader));


	block->is_free = true;

	/* Merge with next if free*/
	if (block->next != NULL
		&& block->next->is_free)
	{
		block->size += block->next->size + sizeof(KMemoryHeader);
		block->next = block->next->next;

		if (block->next)
			block->next->prev = block;
	}

	/* Merge with previous if free */
	if (block->prev != NULL
		&& block->prev->is_free)
	{
		block->prev->size += block->size + sizeof(KMemoryHeader);
		block->prev->next = block->next;
		if (block->next)
			block->next->prev = block->prev;

		if (block == ptr_free)
			ptr_free = block->prev;
	}
}