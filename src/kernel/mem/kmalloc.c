#include <kernel/mem/kmalloc.h>
#include <kernel/mem/pmm.h>
#include <kernel/utils/helpers.h>

#define BLOCK_SIZE 4096			/* 4 KB pages*/

extern uint32_t mem_ammount_b;

static KMemoryHeader* ptr_free= NULL;

static inline void kmalloc_init()
{
	ptr_free = pmm_alloc_block();

	KMemoryHeader* header = (KMemoryHeader*)ptr_free;
	header->next = NULL;
	header->prev = NULL;
	header->size =  mem_ammount_b - sizeof(KMemoryHeader);
	header->is_free = true;
}

void* kmalloc(size_t n_bytes)
{
	if (ptr_free == NULL)
		kmalloc_init();

	KMemoryHeader* ptr_current = ptr_free;
	while (ptr_current)
	{
		/* Large enough? */
		if (! ptr_current->is_free
			|| ptr_current->size < n_bytes + sizeof(KMemoryHeader) + sizeof(size_t))
		{
			ptr_current = ptr_current->next;
			continue;
		}

		/* Yes, Block is large enough */

		KMemoryHeader* new_block = (KMemoryHeader*)((char*)ptr_current + sizeof(KMemoryHeader) + n_bytes);
		new_block->size = ptr_current->size - n_bytes - sizeof(KMemoryHeader);
		new_block->is_free = true;  /* New block is free */

		ptr_current->size = n_bytes;
		ptr_current->is_free = false;

		/* new_block node is beetwen current and next block*/
		new_block->next = ptr_current->next;
		ptr_current->next = new_block;

		new_block->prev = ptr_current;
		if (new_block->next)
			new_block->next->prev = new_block;

		/* on 4K block allocator, reserve the used blocks (yes O(n) by now) */
		for (size_t i = 0; i < n_bytes; i++)
		{
			void *base_ptr = (void*)ptr_current + sizeof(KMemoryHeader) + i;
			base_ptr = align_ptr_down(base_ptr, BLOCK_SIZE);
			pmm_reserve_block((uint32_t)base_ptr / BLOCK_SIZE);
		}

		/* Return pointer to the allocated memory (just after the header) */
		return (void*)((char*)ptr_current + sizeof(KMemoryHeader));
	}

	return NULL;
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
