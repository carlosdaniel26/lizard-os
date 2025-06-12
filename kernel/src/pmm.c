#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <limine.h>
#include <tty.h>
#include <pmm.h>
#include <alias.h>
#include <helpers.h>

#define BLOCK_SIZE 4096			/* 4 KB pages*/
#define BLOCK_SIZE_KB 4			/* 4 KB pages*/

#define MEMORY_AVAILABLE 0
#define MEMORY_RESERVED 1

#define get_absolute_bid(addr) ((uintptr_t)addr / 4096)

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

extern uint8_t kernel_start;
extern uint8_t kernel_end;

uint64_t mem_ammount_b;
uint64_t mem_ammount_kb;
uint8_t *mem_bitmap;
uint8_t *mem_start;
uint32_t bitmap_size;
uint32_t total_blocks;

void handle_mmap()
{
	if (memmap_request.response == NULL)
	{
		kpanic("memmap request empty");
		while(1){}
	}

	for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) 
	{
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) 
        {
            mem_ammount_b += entry->length;
        }
    }

    mem_ammount_kb = mem_ammount_b / 1024;
}

static inline uint32_t pmm_block_number(void *ptr)
{
	if ((uintptr_t)ptr % 4096 == 0)
		return ((uintptr_t)ptr - (uintptr_t)mem_start) / BLOCK_SIZE;


	uintptr_t addr = (uintptr_t)ptr;
	addr = (uintptr_t)align_ptr_down(addr, BLOCK_SIZE);

	uint32_t block_number = (addr - (uintptr_t)mem_start) / BLOCK_SIZE;

	return block_number;
}

static inline uint32_t pmm_block_addr(uint32_t block_number)
{
	return (uintptr_t)mem_start + (block_number * BLOCK_SIZE);
}

#define AVAILABLE 0
#define RESERVED 1

static inline bool pmm_check_block(uint32_t block_number)
{
	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	return ptr_get_bit(mem_bitmap + byte_index, bit_index);
}

void pmm_reserve_block(uint32_t block_number)
{
	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	ptr_set_bit(mem_bitmap + byte_index, bit_index);
}

void pmm_init()
{

	handle_mmap();

	mem_ammount_kb = align_down(mem_ammount_kb, BLOCK_SIZE_KB);

	mem_ammount_b = mem_ammount_kb * 1024;

	total_blocks = mem_ammount_kb / BLOCK_SIZE_KB;
	bitmap_size = total_blocks / (8);

	mem_bitmap = &kernel_end + 1;

	memset(mem_bitmap, 0, bitmap_size); /* init the bitmap*/

	mem_start = (uint8_t*)mem_bitmap + bitmap_size;

	mem_start = align_ptr_up(mem_start, BLOCK_SIZE);

	/**
	 * [1MB][BITMAP][MEM]
	 */
}

void *pmm_alloc_block(uint32_t ammount)
{
	if (ammount == 0)
		return NULL;

	uint32_t free_blocks_in_row = 0;
	uint32_t start_block = 0;

	/* Block Seek */
	for (uint32_t block = 0; block < bitmap_size; block++)
	{
		/* the block are free*/
		if (pmm_check_block(block) == AVAILABLE)
		{
			if (free_blocks_in_row == 0)
				start_block = block;

			free_blocks_in_row++;

			if (free_blocks_in_row == ammount)
			{
				/* Reserved set of blocks */
				for (uint32_t b = start_block; b <= block; b++)
				{
					pmm_reserve_block(b);
				}

				void *addr = (void*)  pmm_block_addr(start_block);

				return addr;
			}
		}
		/* the block are in use*/
		else
		{
			free_blocks_in_row = 0;
			continue;
		}
	}

	return NULL;
}

void *pmm_alloc_blocks()
{
	/* bytes*/
	for (uint32_t block = 0; block < bitmap_size; block++)
	{
		/* the block are free*/
		if (pmm_check_block(block) == AVAILABLE)
		{
			pmm_reserve_block(block);

			void *addr = (void*)  pmm_block_addr(block);

			return addr;
		}
		/* the block are in use*/
		else
		{
			continue;
		}
	}

	return NULL;
}

void pmm_free_block(void *ptr)
{
	uint32_t block_number = pmm_block_number(ptr);

	if ( block_number >= total_blocks)
		return;

	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	ptr_unset_bit(&mem_bitmap[byte_index], bit_index);
}

uint32_t pmm_free_block_count()
{
	uint32_t free_ammount = 0;

	for (uint32_t block = 0; block < total_blocks; block++)
	{
		if (pmm_check_block(block) == AVAILABLE)
			free_ammount++;
	}

	return free_ammount;
}