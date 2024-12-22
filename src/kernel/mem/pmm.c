#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include <kernel/mem/pmm.h>
#include <multiboot2.h>
#include <kernel/terminal/terminal.h>
#include <kernel/utils/alias.h>

#define BLOCK_SIZE 4096			// 4 KB pages
#define BLOCK_SIZE_KB 4			// 4 KB pages

#define MEMORY_AVAILABLE 1
#define MEMORY_RESERVED 0

struct mmap_entry_t {
	uint32_t size; // size exclude itself when stores the size of the struct
	
	uint64_t addr;
	uint64_t len;
	uint32_t type;
} __attribute__((packed));

extern uint32_t kernel_start;
extern uint32_t kernel_size;


uint64_t mem_ammount_kb;
uint8_t *mem_bitmap;
uint8_t *mem_start;
uint32_t bitmap_size;
uint8_t total_blocks;

void detect_memory(struct multiboot_info_t* mb_info)
{
	if (mb_info->flags & 0x01)
	{
		mem_ammount_kb = (mb_info->mem_lower + mb_info->mem_upper);
	}
}

void print_ammount_mem_mb()
{
	printf("mem ammount kb: %u\n", mem_ammount_kb);
	printf("mem_start: %u\n\n", mem_start);
}

uint32_t get_block_number(void *ptr)
{
	return ((uintptr_t)ptr - (uintptr_t)mem_start) / BLOCK_SIZE;
}

void pmm_init(struct multiboot_info_t* mb_info)
{
	detect_memory(mb_info);
	process_memory_map(mb_info);

	total_blocks = mem_ammount_kb / BLOCK_SIZE_KB;
	bitmap_size = total_blocks / (8);
	mem_bitmap = (uint8_t*)(&kernel_start + kernel_size + 1); // start after the kernel

	memset(mem_bitmap, 0, bitmap_size); // init the bitmap

	mem_start = (uint8_t*)mem_bitmap + bitmap_size;
	
	while((((uintptr_t)mem_start) % 4096) != 0)
	{
		mem_start++;
	}

	/**
	 * [1MB][BITMAP][MEM]
	 */

	print_ammount_mem_mb();
}

void *pmm_alloc_block()
{
	for (uint32_t i = 0; i < total_blocks; i++)
	{
		// bits
		for (uint32_t j = 0; j < 8; j++)
		{
			// the block are free
			if (ptr_get_bit(mem_bitmap + i, j) == 0)
			{
				// now in use
				ptr_set_bit(mem_bitmap + i, j);

				return (void*)  (mem_start + (i+j) * BLOCK_SIZE);
			}
			// the block are in use
			else
			{
				continue;
			}
		}
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

void process_memory_map(const struct multiboot_info_t *mb_info)
{
	struct mmap_entry_t *ptr_mmap = (struct mmap_entry_t*)(uintptr_t)	mb_info->mmap_addr;
	uintptr_t mmap_end = mb_info->mmap_addr + mb_info->mmap_length;

	while((uintptr_t)ptr_mmap < mmap_end)
	{
		if (ptr_mmap->type != MEMORY_AVAILABLE)
		{
			unsigned bitmap_byte_index = ptr_mmap->addr / 8;

			for (unsigned i = bitmap_byte_index; i < ptr_mmap->len; i++)
			{
				for (unsigned j = 0; j < 8; j++)
				{
					// set bit used
					ptr_set_bit(mem_bitmap + i, j);
				}
			}

		}
		
		// i++
		ptr_mmap = (struct mmap_entry_t*)((uintptr_t)ptr_mmap + ptr_mmap->size + sizeof(ptr_mmap->size));
	}
}