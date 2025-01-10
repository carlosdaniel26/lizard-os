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
    page_dir[0] = (uintptr_t)page_table | PRESENT_WRITABLE;

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

void map_page(uint32_t from, uint32_t to)
{
    uint32_t page_ammount = to / 4096;
    uint32_t address = from; // First P_Address to map

    for (uint32_t i = 0; i < page_ammount; i++)
    {
        uint32_t page_dir_index = address >> 22; // Get the page number
        if (! (page_dir_index & 1))
        {
            uint32_t *p_table = pmm_alloc_block();  

            page_directory[page_dir_index] = (uintptr_t)p_table | PRESENT_WRITABLE;
        }

        uint32_t page_table_index = (address >> 12) & 0x3FF;

        uint32_t entry = address;
        entry |= PRESENT_WRITABLE;
        page_table[page_table_index] = entry;

        address += PAGE_SIZE_BYTES;
    }
}

void enable_paging() 
{
    alloc_memory_for_tables();
    map_page(0x00, (uint32_t)(&kernel_end + 4096));
    
    enable_paging_registers();
}