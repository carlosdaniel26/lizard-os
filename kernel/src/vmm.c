#include <buddy.h>
#include <debug.h>
#include <early_alloc.h>
#include <framebuffer.h>
#include <helpers.h>
#include <kernelcfg.h>
#include <limine.h>
#include <panic.h>
#include <pgtable.h>

#include <stdio.h>
#include <string.h>
#include <types.h>
#include <vmm.h>

#define DIV_UP(a, b) (((a) + (b) - 1) / (b))

extern u64 kernel_stack[];
extern u32 kernel_start;
extern u32 kernel_end;

u64 *current_pml4 = NULL;
u64 *kpml4 = NULL;
extern u64 hhdm_offset;
extern struct limine_executable_address_request kernel_address_request;

void vmm_init(void)
{
    kpml4 = pgtable_alloc_table();
    current_pml4 = kpml4;

    /* get regions from buddy then maps all the regions */

    u64 vir = kernel_address_request.response->virtual_base;
    u64 phys = kernel_address_request.response->physical_base;
    u64 vstart = align_down((u64)&kernel_start, PAGE_SIZE);
    u64 vend = align_up((u64)&kernel_end, PAGE_SIZE);
    u64 pstart = phys - ((u64)&kernel_start - vstart);

    u64 kernel_pages = (vend - vstart) / PAGE_SIZE;

    pgtable_maprange(kpml4, vstart, pstart, kernel_pages, PAGE_PRESENT | PAGE_WRITABLE);

    char *ptr = (char *)0xffff8000bff7dfd0; /*fault addr */
    char *page = (char *)align_down((uintptr_t)ptr, PAGE_SIZE);
    extern u64 *kpml4;

    pgtable_maprange(kpml4, (uint64_t)framebuffer, (uint64_t)framebuffer - hhdm_offset,
                     framebuffer_length / PAGE_SIZE, PAGE_PRESENT | PAGE_WRITABLE);

    /* map direct mapped memory */

    u64 tables_needed = DIV_UP(highest_addr, PAGE_SIZE);
    if (tables_needed > highest_addr) kpanic("VMM: tables_needed overflow");
    pgtable_maprange(kpml4, hhdm_offset, 0, tables_needed, PAGE_PRESENT | PAGE_WRITABLE);
    pgtable_switch(kpml4);
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