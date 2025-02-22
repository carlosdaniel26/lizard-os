#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <kernel/utils/alias.h>

#include <kernel/mem/vmm.h>
#include <kernel/mem/pmm.h>

#define PRESENT_WRITABLE 0x3
#define PAGE_SIZE_BYTES 4096

uint32_t *page_directory;
uint32_t *page_table;

void enable_paging_registers()
{
	/* Link page table in page directory*/

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
	memset(page_directory, 0, PAGE_SIZE_BYTES);
}

uint32_t count = 0;

void map_page(uint32_t p_addr, uint32_t v_addr)
{
	uint32_t pd_index = v_addr >> 22; /* Get the page number*/

	if (! (page_directory[pd_index] & PRESENT_WRITABLE))
	{
		page_table = pmm_alloc_block();

		page_directory[pd_index] = (uintptr_t)page_table | PRESENT_WRITABLE;
	}

	uint32_t pt_index = (v_addr >> 12) & 0x3FF;
	uint32_t entry = p_addr | PRESENT_WRITABLE;

	page_table[pt_index] = entry;

}

void map_pages(uint32_t p_addr, uint32_t length, uint32_t v_addr)
{
	/* Align Lenght to 4096*/
	while(length % 4096 != 0)
	{
		length++;
	}

	uint32_t page_ammount = length / 4096;

	for (uint32_t i = 0; i < page_ammount; i++)
	{
		map_page(p_addr, v_addr);

		p_addr += PAGE_SIZE_BYTES;
		v_addr += PAGE_SIZE_BYTES;
	}
}

#define identity_paging(addr, length) map_pages(addr, length, addr)

void enable_paging()
{
	extern uint32_t mem_ammount_b;

	alloc_memory_for_tables();
	identity_paging(0x00, mem_ammount_b);


	enable_paging_registers();
}