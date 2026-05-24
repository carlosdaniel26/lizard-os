#include <lizard/init.h>
#include <nolibc/types.h>
#include <lizard/buddy.h>
#include <nolibc/stdio.h>
#include <lizard/helpers.h>
#include <lizard/limine.h>
#include <lizard/pgtable.h>
#include <nolibc/stddef.h>
#include <lizard/gdt.h>
#include <lizard/ktime.h>
#include <lizard/timer.h>

extern struct limine_executable_address_request kernel_address_request;
extern u64 hhdm_offset;
extern u32 kernel_start;

/* Declare time_init here because it's not in ktime.h */
int time_init(void);

void do_initcalls(initcall_t *start, initcall_t *end)
{
    for (const initcall_t *fn = start; fn < end; fn++)
    {
        (*fn)();
    }
}

void __init kernel_bootstrap()
{
    do_initcalls(__initcall0_start, __initcall0_end); /* early */
    do_initcalls(__initcall1_start, __initcall1_end); /* core */
    do_initcalls(__initcall2_start, __initcall2_end); /* postcore */
    do_initcalls(__initcall3_start, __initcall3_end); /* arch */
    do_initcalls(__initcall4_start, __initcall4_end); /* subsystem */
    
    gdt_init_dynamic();

    time_init();
    timer_init();
    timer_start();

    do_initcalls(__initcall5_start, __initcall5_end); /* filesystem */
    do_initcalls(__initcall6_start, __initcall6_end); /* device */
    do_initcalls(__initcall7_start, __initcall7_end); /* late */

    free_init_sections();
}

void free_init_sections()
{
    u64 start = align_down((u64)__init_start, PAGE_SIZE);
    u64 end = align_up((u64)__init_end, PAGE_SIZE);
    u64 pages = (end - start) / PAGE_SIZE;

    u64 phys_base = kernel_address_request.response->physical_base;
    u64 virt_base = (u64)&kernel_start;

    for (u64 addr = start; addr < end; addr += PAGE_SIZE)
    {
        u64 phys = addr - virt_base + phys_base;
        u64 hhdm_addr = phys + hhdm_offset;
        
        buddy_free((void *)hhdm_addr, 0);
    }

    kprintf("INIT: Freed %d pages of initialization code/data\n", (int)pages);
}
