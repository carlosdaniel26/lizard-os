#include <helpers.h>
#include <limine.h>
#include <pmm.h>
#include <stddef.h>
#include <types.h>
#include <string.h>
#include <vmm.h>
#include <framebuffer.h>

__attribute__((used, section(".limine_requests"))) static volatile struct limine_executable_address_request
	kernel_address_request
	= {.id = LIMINE_EXECUTABLE_ADDRESS_REQUEST, .revision = 0};

/* 

x86-64 VIRTUAL ADDRESS LAYOUT

16 bits	   9 bits	 9 bits	   9 bits	 9 bits	   12 bits
┌─────────┬─────────┬─────────┬─────────┬─────────┬────────────┐
│ SIGN	│ PML4	│ PDP		│ PD		│ PT		│ OFFSET	   │
│ EXTEND	│ INDEX	│ INDEX	│ INDEX	│ INDEX	│ (4K PAGE)  │
├─────────┼─────────┼─────────┼─────────┼─────────┼────────────┤
│ 0xFFFF	│ 0x1FF	│ 0x1FF	│ 0x1FF	│ 0x1FF	│ 0xFFF	   │
│ (SIGN)	│ (511)	│ (511)	│ (511)	│ (511)	│ (4095)	   │
└─────────┴─────────┴─────────┴─────────┴─────────┴────────────┘

CANONICAL ADDRESS RANGES:
┌──────────────────────────────────────────────────────────────┐
│ 0x0000000000000000 - 0x00007FFFFFFFFFFF	 │ User Space		   │
├──────────────────────────────────────────────────────────────┤
│ 0xFFFF800000000000 - 0xFFFFFFFFFFFFFFFF	 │ Kernel Space	   │
└──────────────────────────────────────────────────────────────┘

*/

extern u64 stack_start;

extern u32 kernel_start;
extern u32 kernel_end;

static u64 *kernel_pml4;

static inline void invlpg(void *addr)
{
	__asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

static u64 *vmm_alloc_table()
{
	void *page = pmm_alloc_block() + hhdm_offset;
	memset(page, 0, PAGE_SIZE);
	return (u64 *)page;
}

void vmm_map(u64 *pml4, u64 virt, u64 phys, u64 flags)
{
	/* Normalize values */
	virt = (u64)align_ptr_down(virt, PAGE_SIZE);
	phys = (u64)align_ptr_down(phys, PAGE_SIZE);

	/* Calculate index */
	u64 pml4_i = (virt >> 39) & 0x1FF;
	u64 pdpt_i = (virt >> 30) & 0x1FF;
	u64 pd_i = (virt >> 21) & 0x1FF;
	u64 pt_i = (virt >> 12) & 0x1FF;

	u64 *pdpt, *pd, *pt;

	/* PDPT */
	if (!(pml4[pml4_i] & PAGE_PRESENT))
	{
		pdpt = vmm_alloc_table();
		pml4[pml4_i] = ((u64)pdpt - hhdm_offset) | flags;
	} else
	{
		pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);
	}

	/* PD */
	if (!(pdpt[pdpt_i] & PAGE_PRESENT))
	{
		pd = vmm_alloc_table();
		pdpt[pdpt_i] = ((u64)pd - hhdm_offset) | flags;
	} else
	{
		pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);
	}

	/* PT */
	if (!(pd[pd_i] & PAGE_PRESENT))
	{
		pt = vmm_alloc_table();
		pd[pd_i] = ((u64)pt - hhdm_offset) | flags;
	} else
	{
		pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);
	}

	pt[pt_i] = phys | flags;
	invlpg((void *)virt);
}

static inline void vmm_maprange(u64 *pml4, u64 virt, u64 phys, u64 length_in_blocks,
								u64 flags)
{
	for (u64 i = 0; i < length_in_blocks; i++)
	{
		vmm_map(pml4, virt, phys, flags);
		virt += PAGE_SIZE;
		phys += PAGE_SIZE;
	}
}

static inline void vmm_load_pml4()
{
	register u64 phys = (u64)kernel_pml4 - hhdm_offset;
	__asm__ volatile("mov %0, %%cr3" ::"r"(phys));
}

void vmm_init()
{
	kernel_pml4 = vmm_alloc_table();

	/* == DEFAULT MAPPING == */

	vmm_maprange(kernel_pml4, hhdm_offset, 0, total_blocks, PAGE_PRESENT | PAGE_WRITABLE);

	/* == SPECIAL AREA MAPPING == */

	/* Kernel */
	u64 vir = kernel_address_request.response->virtual_base;
	u64 phys = kernel_address_request.response->physical_base;
	vmm_maprange(kernel_pml4, vir, phys, (u64)(&kernel_end - &kernel_start),
				 PAGE_PRESENT | PAGE_WRITABLE);

	vmm_load_pml4();
}

void *vmm_alloc_page(void *pml4)
{
	uintptr_t ptr = (uintptr_t)pmm_alloc_block();
	vmm_map(pml4, ptr + hhdm_offset, ptr, PAGE_PRESENT | PAGE_WRITABLE);

	return (void *)ptr + hhdm_offset;
}

void *vmm_alloc_block_row(void *pml4, u64 ammount)
{
	if (ammount == 0)
		return NULL;
	
	uintptr_t ptr = (uintptr_t)pmm_alloc_block_row(ammount);

	for (u64 i = 0; i < ammount; i++)
	{
		vmm_map(pml4, ptr + hhdm_offset + (i * PAGE_SIZE), ptr + (i * PAGE_SIZE),
				PAGE_PRESENT | PAGE_WRITABLE);
	}

	return (void *)ptr + hhdm_offset;
}

int vmm_free_page(u64 *pml4, uintptr_t ptr)
{
	ptr = (uintptr_t)align_ptr_down(ptr, PAGE_SIZE);

	/* Calculate index */
	u64 pml4_i = (ptr >> 39) & 0x1FF;
	u64 pdpt_i = (ptr >> 30) & 0x1FF;
	u64 pd_i = (ptr >> 21) & 0x1FF;
	u64 pt_i = (ptr >> 12) & 0x1FF;

	u64 *pdpt, *pd, *pt;

	/* Get the table */
	pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);

	pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);

	pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);

	/* PDPT */
	if (!(pml4[pml4_i] & PAGE_PRESENT))
		return -1;

	/* PD */
	if (!(pdpt[pdpt_i] & PAGE_PRESENT))
		return -1;

	/* PT */
	if (!(pd[pd_i] & PAGE_PRESENT))
		return -1;

	/* Not present anymore */
	pt[pt_i] &= (~PAGE_PRESENT | ~PAGE_WRITABLE);

	pmm_free_block((void *)ptr - hhdm_offset);

	return 0;
}