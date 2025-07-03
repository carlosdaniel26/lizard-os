#include <limine.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <helpers.h>
#include <pmm.h>
#include <vmm.h>

__attribute__((used, section(".limine_requests")))
static volatile struct limine_executable_address_request kernel_address_request = {
    .id = LIMINE_EXECUTABLE_ADDRESS_REQUEST,
    .revision = 0
};

extern uint64_t stack_start;


extern uint32_t kernel_start;
extern uint32_t kernel_end;

extern uint64_t hhdm_offset;
extern uint32_t *framebuffer;
extern uint32_t framebuffer_length;
 
static uint64_t *kernel_pml4;

static inline void invlpg(void *addr) {
    __asm__ volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}

static uint64_t *vmm_alloc_table() {
    void *page = pmm_alloc_block();
    memset(page, 0, PAGE_SIZE);
    return (uint64_t *)page;
}

void vmm_init() {
    kernel_pml4 = vmm_alloc_table();
}
//    0xffff80007ff86fd8

void vmm_map(uint64_t virt, uint64_t phys, uint64_t flags)
{
    /* Normalize values */
    virt = (uint64_t)align_ptr_down(virt, PAGE_SIZE);
    phys = (uint64_t)align_ptr_down(phys, PAGE_SIZE);

    /* Calculate index */
    uint64_t pml4_i = (virt >> 39) & 0x1FF;
    uint64_t pdpt_i = (virt >> 30) & 0x1FF;
    uint64_t pd_i   = (virt >> 21) & 0x1FF;
    uint64_t pt_i   = (virt >> 12) & 0x1FF;

    uint64_t *pdpt, *pd, *pt;

    /* PDPT */
    if (! (kernel_pml4[pml4_i] & PAGE_PRESENT))
    {
        pdpt = vmm_alloc_table();
        kernel_pml4[pml4_i] = ((uint64_t)pdpt - hhdm_offset) | flags;
    } 
    else 
    {
        pdpt = (uint64_t *)((kernel_pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);
    }

    /* PD */
    if (! (pdpt[pdpt_i] & PAGE_PRESENT)) 
    {
        pd = vmm_alloc_table();
        pdpt[pdpt_i] = ((uint64_t)pd - hhdm_offset) | flags;
    }
    else 
    {
        pd = (uint64_t *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);
    }

    /* PT */
    if (! (pd[pd_i] & PAGE_PRESENT))
    {
        pt = vmm_alloc_table();
        pd[pd_i] = ((uint64_t)pt - hhdm_offset) | flags;
    } 
    else 
    {
        pt = (uint64_t *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);
    }

    pt[pt_i] = phys | flags;
    invlpg((void *)virt);
}

void vmm_maprange(uint64_t virt, uint64_t phys, uint64_t length_in_blocks, uint64_t flags)
{
    for (uint64_t i = 0; i < length_in_blocks; i++)
    {
        vmm_map(virt, phys, flags);
        virt += PAGE_SIZE;
        phys += PAGE_SIZE;
    }
}

void vmm_load_pml4() 
{
    register uint64_t phys = (uint64_t)kernel_pml4 - hhdm_offset;
    __asm__ volatile("mov %0, %%cr3" :: "r"(phys));
}


void vmm_map_kernel()
{
    uint64_t vir = kernel_address_request.response->virtual_base;
    uint64_t phys = kernel_address_request.response->physical_base;

    vmm_maprange(vir, phys, (uint64_t)(&kernel_end - &kernel_start), PAGE_PRESENT | PAGE_WRITABLE);
}

void vmm_map_framebuffer()
{
    vmm_maprange((uint64_t)framebuffer, (uint64_t)framebuffer - hhdm_offset, framebuffer_length / PAGE_SIZE, PAGE_PRESENT | PAGE_WRITABLE);
}

void vmm_map_stack()
{
    vmm_maprange(stack_start + hhdm_offset, stack_start, 1, PAGE_PRESENT | PAGE_WRITABLE);
}