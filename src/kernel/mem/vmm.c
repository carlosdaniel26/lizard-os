#include <stdint.h>
#include <kernel/mem/pmm.h>

#define PRESENT_WRITABLE 0x3
#define PAGE_SIZE_BYTES 4096

uint32_t *page_directory;
uint32_t *page_table;

extern uint32_t kernel_end;

void enable_paging_registers()
{
	/* Link page table in page directory*/
	uint32_t *page_dir = page_directory;
	page_dir[0] = (uintptr_t)page_table | PRESENT_WRITABLE;

	/* Load the page directory*/
	__asm__ volatile (
		"mov %0, %%eax\n"
		"mov %%eax, %%cr3\n"
		:
		: "r" (page_directory)
		: "%eax"
	);

	/* Enable paging in CR0*/
	__asm__ volatile (
		"mov %%cr0, %%eax\n"
		"or $0x80000001, %%eax\n"
		"mov %%eax, %%cr0\n"
		:
		:
		: "%eax"
	);
}

void alloc_memory_for_tables()
{
	page_directory = pmm_alloc_block();
	page_table = pmm_alloc_block();
}

void map_page(uint32_t p_addr, uint32_t length, uint32_t v_addr)
{
	/* Align Lenght to 4096*/
	while(length % 4096 != 0)
	{
		length++;
	}


	uint32_t page_ammount = length / 4096;

	for (uint32_t i = 0; i < page_ammount; i++)
	{
		uint32_t page_dir_index = v_addr >> 22; /* Get the page number*/
		if (! (page_dir_index & 1))
		{
			uint32_t *p_table = pmm_alloc_block();

			page_directory[page_dir_index] = (uintptr_t)p_table | PRESENT_WRITABLE;
		}

		uint32_t page_table_index = (v_addr >> 12) & 0x3FF;

		uint32_t entry = p_addr;
		entry |= PRESENT_WRITABLE;
		page_table[page_table_index] = entry;

		p_addr += PAGE_SIZE_BYTES;
		v_addr += PAGE_SIZE_BYTES;
	}
}

#define identity_paging(addr, length) map_page(addr, length, addr)

void enable_paging()
{
	kernel_end = (uint32_t)&kernel_end;

	alloc_memory_for_tables();
	identity_paging(0x00, (kernel_end + 4096));

	enable_paging_registers();
}