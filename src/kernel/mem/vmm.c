#include <stdint.h>
#include <string.h>

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t page_table[1024] __attribute__((aligned(4096)));

extern unsigned kernel_end;

void enable_paging_c() 
{
    // Zero out the page directory and page table manually
    memset(page_directory, 0, 4096);
    memset(page_table, 0, 4096);

    // Set up page table for 0x100000 to 0x128000
    uint32_t *edi = page_table;
    uint32_t ecx = (unsigned)(&kernel_end) / 4096; // Number of pages
    uint32_t eax = 0x0; // First P_Address to map

    for (uint32_t i = 0; i < ecx; i++) {
        uint32_t ebx = eax; // Save the address for calculations
        ebx >>= 12; // Get the page number

        uint32_t edx = eax; // Save the address for entry
        edx |= 0x3; // Mark as present and writable

        edi[ebx] = edx; // Write the entry to the page table
        eax += 0x1000; // Move to the next page
    }

    // Link page table in page directory
    uint32_t *page_dir = page_directory;
    page_dir[0] = (uint32_t)page_table | 0x3; // Mark as present and writable

    // Load the page directory into CR3
    __asm__ volatile (
        "mov %0, %%eax\n"
        "mov %%eax, %%cr3\n"
        :
        : "r" (page_directory)
        : "%eax"
    );

    // Enable paging by setting PG bit in CR0
    __asm__ volatile (
        "mov %%cr0, %%eax\n"
        "or $0x80000001, %%eax\n"
        "mov %%eax, %%cr0\n"
        :
        :
        : "%eax"
    );
}

