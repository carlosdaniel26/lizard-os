#include <stdint.h>
#include <kernel/mem/pmm.h>

#define PRESENT_WRITABLE 0x3
#define PAGE_SIZE_BYTES 4096

uint32_t *page_directory;
uint32_t *page_table;

extern uint32_t kernel_end;

void enable_paging_registers()
{
    // Link page table in page directory
    uint32_t *page_dir = page_directory;
    page_dir[0] = (uint32_t)page_table | PRESENT_WRITABLE;

    // Load the page directory
    __asm__ volatile (
        "mov %0, %%eax\n"
        "mov %%eax, %%cr3\n"
        :
        : "r" (page_directory)
        : "%eax"
    );

    // Enable paging in CR0
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

void enable_paging() 
{
    alloc_memory_for_tables();

    uint32_t page_ammount = (uint32_t)(&kernel_end + 4096) / 4096;
    uint32_t address = 0x0; // First P_Address to map

    for (uint32_t i = 0; i < page_ammount; i++) 
    {
        uint32_t page_dir_index = address >> 12; // Get the page number

        uint32_t entry = address;
        entry |= PRESENT_WRITABLE;
        page_table[page_dir_index] = entry;

        address += PAGE_SIZE_BYTES;
    }

    enable_paging_registers();
}