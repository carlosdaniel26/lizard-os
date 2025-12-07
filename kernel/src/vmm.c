#include <helpers.h>
#include <limine.h>
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

void vmm_init(void)
{
    kpml4 = pgtable_alloc_table();
	current_pml4 = kpml4;

    vmm_map_page((u64)kpml4, (u64)kpml4 - hhdm_offset, PAGE_PRESENT | PAGE_WRITABLE);
    /* get regions from buddy then maps all the regions */
    for (MemoryRegion *region = regions; region != NULL; region = region->next)
    {
        if (region->type != MEMORY_REGION_USABLE)
            continue;

        u64 region_start = region->base;
        u64 region_end = region->base + region->length;
        u64 region_pages = (region_end - region_start + PAGE_SIZE - 1) / PAGE_SIZE;

        /* check if maps ffffffff7f80ebb7, print yes or no and hlt*/
        debug_printf("VMM: Mapping memory region: start=0x%x, end=0x%x, pages=%u\n",
                         region_start, region_end, region_pages);
        if (region_start > 0xbfd09000 || region_end < 0xbfd09000)
        {
            debug_printf("VMM: Memory region overlaps critical address 0xbfd09000\n");
            hlt();
        }

        pgtable_maprange(kpml4, region_start + hhdm_offset, region_start,
                        region_pages, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
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
    uintptr_t ptr = (uintptr_t)buddy_alloc(0);
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