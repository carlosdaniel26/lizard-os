/**
 * https://en.wikipedia.org/wiki/Control_register
 * https://youtu.be/oD1_3iL12mU?si=d3vTPy--fcKyRxD6
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <kernel/mem/pmm.h>
#include <kernel/mem/vmm.h>

#define TABLE_SIZE 1024
#define PAGE_SIZE  4096

extern uint32_t kernel_start;
extern uint32_t kernel_end;
extern uint32_t kernel_size;

uint32_t *p4_table;
uint32_t *p3_table;

void set_cr3() 
{
    __asm__ volatile("mov %0, %%cr3" : : "r" (p4_table));
}

void set_cr0() 
{
    uint32_t cr0_value;

    __asm__ volatile("mov %%cr0, %0" : "=r" (cr0_value));

    cr0_value |= 0x80000001;

    __asm__ volatile("mov %0, %%cr0" : : "r" (cr0_value));
}

void configure_paging_directories()
{
    // give chunks for the pages
    p4_table = pmm_alloc_block();
    p3_table = pmm_alloc_block();
    
    uint32_t page_directory;
    
    /* p4[0] -> p3 */
    page_directory = (uint32_t)p3_table & 0xFFFFF000; // get the address align to 4KB
    page_directory |= 0b11; // set present and write bits

    p4_table[0] = page_directory;

}

typedef uint32_t v_addr;
void idpaging(uint32_t *first_pte, v_addr from, unsigned size)
{

    for(; size > 0; from += PAGE_SIZE, size -= PAGE_SIZE, first_pte++)
    {
        *first_pte = from | 1; // page present
    }
}

void init_paging()
{   
    printf("initing paging...\n");
	configure_paging_directories();
	idpaging(p3_table, (uint32_t)&kernel_start, (&kernel_end) - (&kernel_start));
	set_cr3();
    set_cr0();
    //void (*higher_half_entry)() = (void (*)())0xC0000000;
    //higher_half_entry();
}
