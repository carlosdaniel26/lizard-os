#include <helpers.h>
#include <limine.h>
#include <pmm.h>
#include <stddef.h>
#include <types.h>
#include <string.h>
#include <stdio.h>
#include <vmm.h>
#include <framebuffer.h>
#include <pgtable.h>
#include <buddy.h>

__attribute__((used, section(".limine_requests"))) static volatile struct limine_executable_address_request
    kernel_address_request
    = {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 0};

extern u64 stack_start;
extern u32 kernel_start;
extern u32 kernel_end;

u64 *current_pml4 = NULL;
u64 *kpml4 = NULL;
extern u64 hhdm_offset;
extern u64 total_blocks;

void vmm_init(void)
{
    kpml4 = pgtable_alloc_table();
	current_pml4 = kpml4;

    pgtable_maprange(kpml4, hhdm_offset, 0, total_blocks, PAGE_PRESENT | PAGE_WRITABLE);

    u64 vir = kernel_address_request.response->virtual_base;
    u64 phys = kernel_address_request.response->physical_base;
    u64 kernel_size = (u64)&kernel_end - (u64)&kernel_start;
    u64 kernel_pages = (kernel_size + PAGE_SIZE - 1) / PAGE_SIZE;
    
    pgtable_maprange(kpml4, vir, phys, kernel_pages, PAGE_PRESENT | PAGE_WRITABLE);
    
    pgtable_maprange(kpml4, (uint64_t)framebuffer, (uint64_t)framebuffer - hhdm_offset,
                    framebuffer_length / PAGE_SIZE, PAGE_PRESENT | PAGE_WRITABLE);
    pgtable_switch(kpml4);
    debug_printf("VMM: Initialized.\n");
}

void vmm_map_page(u64 virt, u64 phys, u64 flags)
{
    pgtable_map(current_pml4, virt, phys, flags);
}

void vmm_unmap_page(u64 virt)
{
    pgtable_unmap(current_pml4, virt);
}

void *vmm_alloc_page(void)
{
    char *ptr = buddy_alloc(0);
    pgtable_map(current_pml4, ptr, ptr - hhdm_offset, PAGE_PRESENT | PAGE_WRITABLE);
    return (void *)ptr;
}

void vmm_free_page(void *ptr)
{
    pgtable_unmap(current_pml4, (u64)ptr);
}

void vmm_switch_pml4(u64 *pml4)
{
    current_pml4 = pml4;
    pgtable_switch(pml4);
}