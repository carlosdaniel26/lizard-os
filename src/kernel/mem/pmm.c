#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <kernel/mem/pmm.h>
#include <multiboot2.h>
#include <kernel/terminal/tty.h>
#include <kernel/utils/alias.h>
#include <kernel/utils/helpers.h>
#include <multiboot2.h>

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

uint32_t mem_ammount_b;
uint32_t mem_ammount_kb;
uint8_t *mem_bitmap;
uint8_t *mem_start;
uint32_t bitmap_size;
uint32_t total_blocks;

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

static inline void handle_mmap()
{
	extern struct multiboot_tag_mmap *mmap_tag;
	extern struct multiboot_tag *tag;

	struct multiboot_mmap_entry *mmap;

	if (! mmap_tag)
		return;
	if (! tag)
		return;
	
	for (mmap = mmap_tag->entries;
			(unsigned char*) mmap < (unsigned char*) mmap_tag + mmap_tag->size;
			mmap = (struct multiboot_mmap_entry *) ((unsigned long) mmap + mmap_tag->entry_size))
	{
		if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE)
			continue;

		uintptr_t base = mmap->addr;
		uintptr_t length = mmap->len;

		/* Reserve the blocks*/
		for (uintptr_t addr = base; addr < base + length; addr += 0x1000)
		{
			uint32_t id = pmm_block_number((void*)addr);

			pmm_reserve_block(id);
		}
	}
}

void kprint_ammount_mem_mb()
{
	/*kprintf("mem ammount: %uKB\n", mem_ammount_kb);*/
	kprintf("mem ammount: %uGB\n", mem_ammount_kb / 1024);
	kprintf("blocks: %u\n", total_blocks);
	kprintf("bitmap_size: %uB\n", bitmap_size);
	kprintf("mem_bitmap: %uB\n", mem_bitmap);
	kprintf("mem_start: %uB\n\n", mem_start);
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

	mem_ammount_kb = align_down(mem_ammount_kb, BLOCK_SIZE_KB);

	mem_ammount_b = mem_ammount_kb * 1024;

	total_blocks = mem_ammount_kb / BLOCK_SIZE_KB;
	bitmap_size = total_blocks / (8);

	mem_bitmap = &kernel_end + 1;

	memset(mem_bitmap, 0, bitmap_size); /* init the bitmap*/

	mem_start = (uint8_t*)mem_bitmap + bitmap_size;

	mem_start = align_ptr_up(mem_start, BLOCK_SIZE);

	handle_mmap();

	/**
	 * [1MB][BITMAP][MEM]
	 */
	tty_clean();
	kprint_ammount_mem_mb();
}

void *pmm_alloc_block()
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
