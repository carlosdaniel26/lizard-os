#include <buddy.h>
#include <helpers.h>
#include <pgtable.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <types.h>

/* times i forgot to automap: 3.5*/
/* the .5 is for now that the system is missing one page for some reason, so its kinda my fault */

extern u64 hhdm_offset;

static inline void pgtable_invlpg(void *addr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

u64 *pgtable_alloc_table(void)
{
    char *page = buddy_alloc(0);
    memset(page, 0, PAGE_SIZE);
    return (u64 *)page;
}

void pgtable_free_table(u64 *table)
{
    if (table == NULL) return;
    buddy_free(table, 0);
}

static int pgtable_table_empty(u64 *table)
{
    for (u64 i = 0; i < 512; i++)
    {
        if (table[i] & PAGE_PRESENT)
        {
            return 0;
        }
    }
    return 1;
}

void pgtable_map(u64 *pml4, u64 virt, u64 phys, u64 flags)
{

    virt = (u64)align_ptr_down(virt, PAGE_SIZE);
    phys = (u64)align_ptr_down(phys, PAGE_SIZE);

    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i = (virt >> 21) & 0x1FF;
    u64 pt_i = (virt >> 12) & 0x1FF;

    u64 *pdpt, *pd, *pt;

    if (!(pml4[pml4_i] & PAGE_PRESENT))
    {
        pdpt = pgtable_alloc_table(void);
        pml4[pml4_i] = ((u64)pdpt - hhdm_offset) | flags;
    }
    else
    {
        pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);
    }

    if (!(pdpt[pdpt_i] & PAGE_PRESENT))
    {
        pd = pgtable_alloc_table(void);
        pdpt[pdpt_i] = ((u64)pd - hhdm_offset) | flags;
    }
    else
    {
        pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);
    }

    if (!(pd[pd_i] & PAGE_PRESENT))
    {
        pt = pgtable_alloc_table(void);
        pd[pd_i] = ((u64)pt - hhdm_offset) | flags;
    }
    else
    {
        pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);
    }

    pt[pt_i] = phys | flags;
    pgtable_invlpg((void *)virt);
}

void pgtable_maprange(u64 *pml4, u64 virt, u64 phys, u64 length, u64 flags)
{
    /* check if maps ffffffff7f80ebb7, print yes or no and hlt*/

    for (u64 i = 0; i <= length; i++)
    {
        // debug_printf("Mapping page: virt=0x%x phys=0x%x\n", virt, phys);
        pgtable_map(pml4, virt, phys, flags);
        virt += PAGE_SIZE;
        phys += PAGE_SIZE;
    }
}

void pgtable_unmap(u64 *pml4, u64 virt)
{
    virt = (u64)align_ptr_down(virt, PAGE_SIZE);

    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i = (virt >> 21) & 0x1FF;
    u64 pt_i = (virt >> 12) & 0x1FF;

    u64 *pdpt, *pd, *pt;

    pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);
    pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);
    pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);

    if (!(pml4[pml4_i] & PAGE_PRESENT) || !(pdpt[pdpt_i] & PAGE_PRESENT) || !(pd[pd_i] & PAGE_PRESENT))
        return;

    pt[pt_i] = 0;
    pgtable_invlpg((void *)virt);
    buddy_free((void *)virt - hhdm_offset, 0);

    if (pgtable_table_empty(pt))
    {
        pd[pd_i] = 0;
        pgtable_invlpg(pd);
        pgtable_free_table(pt);

        if (pgtable_table_empty(pd))
        {
            pdpt[pdpt_i] = 0;
            pgtable_invlpg(pdpt);
            pgtable_free_table(pd);

            if (pgtable_table_empty(pdpt))
            {
                pml4[pml4_i] = 0;
                pgtable_invlpg(pml4);
                pgtable_free_table(pdpt);
            }
        }
    }
}

void pgtable_switch(u64 *pml4)
{
    register u64 phys = (u64)pml4 - hhdm_offset;
    __asm__ volatile("mov %0, %%cr3" ::"r"(phys));
}

int pgtable_is_mapped(u64 *pml4, u64 virt)
{
    virt = (u64)align_ptr_down(virt, PAGE_SIZE);

    u64 pml4_i = (virt >> 39) & 0x1FF;
    u64 pdpt_i = (virt >> 30) & 0x1FF;
    u64 pd_i = (virt >> 21) & 0x1FF;
    u64 pt_i = (virt >> 12) & 0x1FF;

    u64 *pdpt, *pd, *pt;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) return 0;

    pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);

    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) return 0;

    pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);

    if (!(pd[pd_i] & PAGE_PRESENT)) return 0;

    pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);

    if (!(pt[pt_i] & PAGE_PRESENT)) return 0;

    return 1;
}