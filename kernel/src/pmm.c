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

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

extern uint8_t kernel_start;
extern uint8_t kernel_end;


uint64_t hhdm_offset;
uint64_t mem_ammount_b;
uint64_t mem_ammount_kb;
uint8_t *mem_bitmap;
uint8_t *mem_start;
uint32_t bitmap_size;
block_id_t total_blocks;


static inline block_id_t pmm_block_number(void *ptr)
{
	if ((uintptr_t)ptr % 4096 == 0)
		return ((uintptr_t)ptr - (uintptr_t)mem_start) / BLOCK_SIZE;


	uintptr_t addr = (uintptr_t)ptr;
	addr = (uintptr_t)align_ptr_down(addr, BLOCK_SIZE);

	uint32_t block_number = (addr - (uintptr_t)mem_start) / BLOCK_SIZE;

	return block_number;
}

static inline uintptr_t pmm_block_addr(block_id_t block_number)
{
	return (uintptr_t)mem_start + (block_number * BLOCK_SIZE);
}

/*
 * This Function bellow has a lot of code to optmize.
 * Mainly on loops, work is needed, fix this on free time.
 */

void handle_mmap()
{
	if (memmap_request.response == NULL)
	{
		kpanic("memmap request empty");
		while(1){}
	}

	if (hhdm_request.response == NULL)
	{
		kpanic("hhdm request empty");
		while(1){}
	}

	hhdm_offset = hhdm_request.response->offset;

	/* Count Free Bytes */
	for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) 
	{
	    struct limine_memmap_entry *entry = memmap_request.response->entries[i];

	    if (entry->type == LIMINE_MEMMAP_USABLE) {
	        mem_ammount_b += entry->length;
	    }
	}

	mem_ammount_kb = mem_ammount_b / 1024;
	total_blocks = mem_ammount_kb / BLOCK_SIZE_KB;
	bitmap_size = total_blocks / 8;


	int mem_bitmap_set;

	/* Find area for the membitmap and then restart loop to reserve the blocks */
	for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) 
	{
	    struct limine_memmap_entry *entry = memmap_request.response->entries[i];

	    if (entry->type != LIMINE_MEMMAP_USABLE)
	        continue;

	    if (entry->length >= bitmap_size + 4096) 
	    {
	        mem_bitmap = (uint8_t*)(entry->base + hhdm_offset);
	        mem_bitmap_set = 1;
	        break;
	    }
	}

	/* Then Reserve Unusable Addresses */
	for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) 
	{
	    struct limine_memmap_entry *entry = memmap_request.response->entries[i];

	    if (entry->type != LIMINE_MEMMAP_USABLE)
	    {
	    	for (uint64_t j = entry->base; j < entry->length; j++)
	    	{
	    		pmm_reserve_block(pmm_block_number((void*)j + hhdm_offset));
	    	}
	   	}
	}

	/* Reserve the blocks used by the bitmap */
	for (uint64_t i = (uint64_t)mem_start; i < bitmap_size; i++) 
	{
	    pmm_reserve_block(pmm_block_number((void*)i + hhdm_offset));	
	}

	/* Reserve the blocks used by the kernel ELF */
	for (uint64_t i = (uint64_t)&kernel_start; i < (uint64_t)&kernel_end; i++) 
	{
	    pmm_reserve_block(pmm_block_number((void*)i + hhdm_offset));	
	}

	/* Exception */
	if (! mem_bitmap_set)
	{
		kpanic("hhdm request empty");
		while(1){}
	}
}

static inline bool pmm_check_block(block_id_t block_number)
{
	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	return ptr_get_bit(mem_bitmap + byte_index, bit_index);
}

void pmm_reserve_block(block_id_t block_number)
{
	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	ptr_set_bit(mem_bitmap + byte_index, bit_index);
}

void pmm_init()
{

	handle_mmap();	

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
	block_id_t start_block = 0;

	/* Block Seek */
	for (uint32_t block = 0; block < bitmap_size; block++)
	{
		/* the block are free*/
		if (pmm_check_block(block) == MEMORY_AVAILABLE)
		{
			if (free_blocks_in_row == 0)
				start_block = block;

			free_blocks_in_row++;

			if (free_blocks_in_row == ammount)
			{
				/* Reserved set of blocks */
				for (block_id_t b = start_block; b <= block; b++)
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
		if (pmm_check_block(block) == MEMORY_AVAILABLE)
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
	block_id_t block_number = pmm_block_number(ptr);

	if ( block_number >= total_blocks)
		return;

	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	ptr_unset_bit(&mem_bitmap[byte_index], bit_index);
}

block_id_t pmm_free_block_count()
{
	block_id_t free_ammount = 0;

	for (block_id_t block = 0; block < total_blocks; block++)
	{
		if (pmm_check_block(block) == MEMORY_AVAILABLE)
			free_ammount++;
	}

	return free_ammount;
}