#include <lizard/buddy.h>
#include <lizard/helpers.h>
#include <lizard/pgtable.h>

#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <nolibc/types.h>

/* times i forgot to automap: 3.5*/
/* the .5 is for now that the system is missing one page for some reason, so its kinda my fault */

extern vaddr_t kernel_pml4;

vaddr_t pgtable_create(void)
{
    vaddr_t pml4_addr = pgtable_alloc_table();
    u64 *pml4 = (u64 *)pml4_addr;
    u64 *k_pml4 = (u64 *)kernel_pml4;

    // Map only the higher-half kernel memory (index 256-511)
    for (int i = KERNEL_PML4_INDEX; i < 512; i++)
    {
        pml4[i] = k_pml4[i];
    }
    return pml4_addr;
}


static inline void pgtable_invlpg(void *addr)
{
    __asm__ volatile("invlpg (%0)" : : "r"(addr) : "memory");
}

vaddr_t pgtable_alloc_table(void)
{
    void *page = (void *)buddy_alloc(0);
    memset(page, 0, PAGE_SIZE);
    return (vaddr_t)page;
}

void pgtable_free(vaddr_t vaddr)
{
    if (vaddr == 0) return;
    buddy_free(vaddr, 0);
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

void pgtable_map(vaddr_t pml4_addr, vaddr_t vaddr, paddr_t paddr, u64 flags)
{
    u64 *pml4 = (u64 *)pml4_addr;
    vaddr = (vaddr_t)align_ptr_down(vaddr, PAGE_SIZE);
    paddr = (paddr_t)align_ptr_down(paddr, PAGE_SIZE);

    u64 pml4_i = (vaddr >> 39) & 0x1FF;
    u64 pdpt_i = (vaddr >> 30) & 0x1FF;
    u64 pd_i = (vaddr >> 21) & 0x1FF;
    u64 pt_i = (vaddr >> 12) & 0x1FF;

    u64 *pdpt, *pd, *pt;

    if (!(pml4[pml4_i] & PAGE_PRESENT))
    {
        pdpt = (u64 *)pgtable_alloc_table();
        pml4[pml4_i] = ((u64)pdpt - hhdm_offset) | flags;
    }
    else
    {
        pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);
    }

    if (!(pdpt[pdpt_i] & PAGE_PRESENT))
    {
        pd = (u64 *)pgtable_alloc_table();
        pdpt[pdpt_i] = ((u64)pd - hhdm_offset) | flags;
    }
    else
    {
        pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);
    }

    if (!(pd[pd_i] & PAGE_PRESENT))
    {
        pt = (u64 *)pgtable_alloc_table();
        pd[pd_i] = ((u64)pt - hhdm_offset) | flags;
    }
    else
    {
        pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);
    }

    pt[pt_i] = paddr | flags;
    pgtable_invlpg((void *)vaddr);
}

void pgtable_maprange(vaddr_t pml4, vaddr_t vaddr, paddr_t paddr, u64 pages, u64 flags)
{
    for (u64 i = 0; i < pages; i++)
    {
        pgtable_map(pml4, vaddr, paddr, flags);
        vaddr += PAGE_SIZE;
        paddr += PAGE_SIZE;
    }
}

void *pgtable_kva(vaddr_t pml4_addr, vaddr_t vaddr)
{
    u64 *pml4 = (u64 *)pml4_addr;
    u64 off = vaddr & 0xFFFUL;
    u64 pml4_i = (vaddr >> 39) & 0x1FF;
    u64 pdpt_i = (vaddr >> 30) & 0x1FF;
    u64 pd_i = (vaddr >> 21) & 0x1FF;
    u64 pt_i = (vaddr >> 12) & 0x1FF;

    if (!(pml4[pml4_i] & PAGE_PRESENT)) return NULL;
    u64 *pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);
    if (!(pdpt[pdpt_i] & PAGE_PRESENT)) return NULL;
    u64 *pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);
    if (!(pd[pd_i] & PAGE_PRESENT)) return NULL;
    u64 *pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);
    if (!(pt[pt_i] & PAGE_PRESENT)) return NULL;

    return (void *)((pt[pt_i] & ~0xFFFUL) + hhdm_offset + off);
}

void pgtable_unmap(vaddr_t pml4_addr, vaddr_t vaddr)
{
    u64 *pml4 = (u64 *)pml4_addr;
    vaddr = (vaddr_t)align_ptr_down(vaddr, PAGE_SIZE);

    u64 pml4_i = (vaddr >> 39) & 0x1FF;
    u64 pdpt_i = (vaddr >> 30) & 0x1FF;
    u64 pd_i = (vaddr >> 21) & 0x1FF;
    u64 pt_i = (vaddr >> 12) & 0x1FF;

    u64 *pdpt, *pd, *pt;

    pdpt = (u64 *)((pml4[pml4_i] & ~0xFFFUL) + hhdm_offset);
    pd = (u64 *)((pdpt[pdpt_i] & ~0xFFFUL) + hhdm_offset);
    pt = (u64 *)((pd[pd_i] & ~0xFFFUL) + hhdm_offset);

    if (!(pml4[pml4_i] & PAGE_PRESENT) || !(pdpt[pdpt_i] & PAGE_PRESENT) || !(pd[pd_i] & PAGE_PRESENT) || !(pt[pt_i] & PAGE_PRESENT))
        return;

    paddr_t paddr = pt[pt_i] & ~0xFFFUL;
    pt[pt_i] = 0;
    pgtable_invlpg((void *)vaddr);
    buddy_free(paddr + hhdm_offset, 0);

    if (pgtable_table_empty(pt))
    {
        pd[pd_i] = 0;
        pgtable_invlpg(pd);
        pgtable_free((vaddr_t)pt);

        if (pgtable_table_empty(pd))
        {
            pdpt[pdpt_i] = 0;
            pgtable_invlpg(pdpt);
            pgtable_free((vaddr_t)pd);

            if (pgtable_table_empty(pdpt))
            {
                pml4[pml4_i] = 0;
                pgtable_invlpg(pml4);
                pgtable_free((vaddr_t)pdpt);
            }
        }
    }
}

/* Recursively free a task's page-table tree: every PT/PD/PDPT page plus the
 * leaf frames they map, then the PML4 itself. Only the user half of the PML4
 * (entries 0..KERNEL_PML4_INDEX-1) is walked - entries 256..511 are the shared
 * higher-half kernel mapping and must NOT be freed. `level`: 3=PML4 2=PDPT
 * 1=PD 0=PT. */
static void pgtable_free_level(u64 *table, int level)
{
    int end = (level == 3) ? KERNEL_PML4_INDEX : 512;

    for (int i = 0; i < end; i++)
    {
        u64 e = table[i];
        if (!(e & PAGE_PRESENT)) continue;

        vaddr_t child = (e & ~0xFFFUL) + hhdm_offset;

        if (level == 0)
        {
            buddy_free(child, 0); /* leaf: a user data page */
        }
        else if ((e & PAGE_HUGE))
        {
            /* task page tables never use huge pages; skip rather than
             * mis-free a single frame of a 2 MiB/1 GiB mapping */
        }
        else
        {
            pgtable_free_level((u64 *)child, level - 1);
            pgtable_free(child);
        }
        table[i] = 0;
    }
}

void pgtable_free_tree(vaddr_t pml4_addr)
{
    if (!pml4_addr) return;
    pgtable_free_level((u64 *)pml4_addr, 3);
    pgtable_free(pml4_addr);
}

void pgtable_switch(vaddr_t pml4)
{
    register paddr_t paddr = (paddr_t)pml4 - hhdm_offset;
    __asm__ volatile("mov %0, %%cr3" ::"r"(paddr));
}

int pgtable_is_mapped(vaddr_t pml4_addr, vaddr_t vaddr)
{
    u64 *pml4 = (u64 *)pml4_addr;
    vaddr = (vaddr_t)align_ptr_down(vaddr, PAGE_SIZE);

    u64 pml4_i = (vaddr >> 39) & 0x1FF;
    u64 pdpt_i = (vaddr >> 30) & 0x1FF;
    u64 pd_i = (vaddr >> 21) & 0x1FF;
    u64 pt_i = (vaddr >> 12) & 0x1FF;

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