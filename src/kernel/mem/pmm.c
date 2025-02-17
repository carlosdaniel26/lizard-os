#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/mem/pmm.h>
#include <multiboot2.h>
#include <kernel/terminal/terminal.h>
#include <kernel/utils/alias.h>

#define BLOCK_SIZE 4096			/* 4 KB pages*/
#define BLOCK_SIZE_KB 4			/* 4 KB pages*/

#define MEMORY_AVAILABLE 1
#define MEMORY_RESERVED 0

#define get_absolute_bid(addr) ((uintptr_t)addr / 4096)

struct mmap_entry_t {
	uint32_t size; /* size exclude itself when stores the size of the struct*/

	uint64_t addr;
	uint64_t len;
	uint32_t type;
} __attribute__((packed));

extern uint8_t kernel_start;
extern uint8_t kernel_end;

uint64_t mem_ammount_b;
uint64_t mem_ammount_kb;
uint8_t *mem_bitmap;
uint8_t *mem_start;
uint32_t bitmap_size;
uint32_t total_blocks;

void kprint_ammount_mem_mb()
{
	kprintf("mem ammount: %uKB\n", mem_ammount_kb);
	kprintf("blocks: %u\n", total_blocks);
	kprintf("bitmap_size: %uB\n", bitmap_size);
	kprintf("mem_bitmap: %uB\n", mem_bitmap);
	kprintf("mem_start: %uB\n\n", mem_start);
}

uint32_t get_block_number(void *ptr)
{
	return ((uintptr_t)ptr - (uintptr_t)mem_start) / BLOCK_SIZE;
}

void pmm_init()
{
	/* Align */
	while(mem_ammount_kb % 4095 != 0)
	{
		mem_ammount_kb--;
	}

	total_blocks = mem_ammount_kb / BLOCK_SIZE_KB;
	bitmap_size = total_blocks / (8);

	mem_bitmap = &kernel_end + 1;

	memset(mem_bitmap, 0, bitmap_size); /* init the bitmap*/

	mem_start = (uint8_t*)mem_bitmap + bitmap_size;

	/* Align 4096 */
	while((uintptr_t)mem_start % 4096 != 0)
	{
		mem_start++;
	}

	/**
	 * [1MB][BITMAP][MEM]
	 */
	terminal_clean();
	kprint_ammount_mem_mb();
}

void *pmm_alloc_block()
{
	uint32_t block_index = 1;

	/* bytes*/
	for (uint32_t i = 0; i < bitmap_size; i++)
	{
		/* bits*/
		for (uint32_t j = 0; j < 8; j++)
		{
			/* the block are free*/
			if (ptr_get_bit(mem_bitmap + i, j) == 0)
			{
				/* now in use*/
				ptr_set_bit(mem_bitmap + i, j);

				void *addr = (void*)  (mem_start + (block_index * BLOCK_SIZE));

				return addr;
			}
			/* the block are in use*/
			else
			{
				continue;
			}
		}
		block_index ++;
	}

	return NULL;
}

void pmm_free_block(void *ptr)
{
	uint32_t block_number = get_block_number(ptr);

	if ( block_number >= total_blocks)
		return;

	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	ptr_unset_bit(&mem_bitmap[byte_index], bit_index);
}