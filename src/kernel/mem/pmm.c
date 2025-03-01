#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <kernel/mem/pmm.h>
#include <multiboot2.h>
#include <kernel/terminal/terminal.h>
#include <kernel/utils/alias.h>
#include <kernel/utils/helpers.h>

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

typedef struct free_node {
	uintptr_t addr;
	size_t lenght;

	struct free_node *next;
} free_node;

free_node *free_list = NULL;

void kprint_ammount_mem_mb()
{
	/*kprintf("mem ammount: %uKB\n", mem_ammount_kb);*/
	kprintf("mem ammount: %uGB\n", mem_ammount_kb / 1024);
	kprintf("blocks: %u\n", total_blocks);
	kprintf("bitmap_size: %uB\n", bitmap_size);
	kprintf("mem_bitmap: %uB\n", mem_bitmap);
	kprintf("mem_start: %uB\n\n", mem_start);
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

static inline void pmm_reserve_block(uint32_t block_number)
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

	/**
	 * [1MB][BITMAP][MEM]
	 */
	terminal_clean();
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

void pmm_free_block(void *ptr)
{
	uint32_t block_number = pmm_block_number(ptr);

	if ( block_number >= total_blocks)
		return;

	uint32_t byte_index = block_number / 8;
	uint32_t bit_index  = block_number % 8;

	ptr_unset_bit(&mem_bitmap[byte_index], bit_index);
}

void kmalloc_init()
{
    free_list = pmm_alloc_block();

    free_list->next = 0;
    free_list->addr = (uintptr_t)free_list + sizeof(free_node);
    free_list->lenght = mem_ammount_b - (uintptr_t)free_list->next - sizeof(free_node);


    kprintf("free_list: %u\n", (uintptr_t)free_list);
    kprintf("free_list->addr: %u\n", free_list->addr);
    kprintf("free_list->lenght: %u\n", free_list->lenght);
    kprintf("mem_ammount_b:     %u\n", mem_ammount_b);
}

void *kmalloc(size_t size)
{
	struct free_node *current = free_list;

	while(current->addr != 0)
	{
		/* Can Alloc on this Area */
		if (current->lenght > size)
		{
			current->addr += size;
			current->lenght -= size;

			uint32_t block = pmm_block_number((void*)current->addr);
			pmm_reserve_block(block);

			return (void*)current->addr;
		}
		
		/* current->next->next->next... */
		if (current->next == NULL)
		{
			break;
		}

		current = current->next;
	}

	return NULL;
}