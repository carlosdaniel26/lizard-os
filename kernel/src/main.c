#include <alias.h>
#include <ata.h>
#include <buddy.h>
#include <cpuid.h>
#include <early_alloc.h>
#include <fat16.h>
#include <framebuffer.h>
#include <gdt.h>
#include <helpers.h>
#include <idt.h>
#include <kernelcfg.h>
#include <keyboard.h>
#include <kmalloc.h>
#include <ktime.h>
#include <limine.h>
#include <panic.h>
#include <pgtable.h>
#include <pic.h>
#include <pit.h>
#include <rtc.h>
#include <ss.h>
#include <stdbool.h>

#include <init.h>
#include <setup.h>
#include <stack.h>
#include <stdio.h>
#include <syscall.h>
#include <tty.h>
#include <types.h>
#include <vfs.h>
#include <vmm.h>

/*
 * feel dumb is temporary, the progress of commits on this
 * project is isn't so feel proud for every commit, even tho the problem is not solved.
 *
 * because its not solved yet. - Carlos, 03:46 30th December, 2025
 */

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

u8 kernel_stack[KERNEL_STACK_SIZE];

// Refactored initcalls from kmain
early_initcall(setup_params);
early_initcall(init_framebuffer);
early_initcall(tty_initialize);
early_initcall(early_alloc_init);

core_initcall(init_cpuid);
core_initcall(PIC_remap);
core_initcall(init_gdt);
core_initcall(init_idt);
core_initcall(buddy_init);

postcore_initcall(vmm_init);

subsys_initcall(task_init);

device_initcall(pit_init);
device_initcall(init_keyboard);
device_initcall(time_init_from_rtc);

device_initcall(syscall_init);
device_initcall(enable_scheduler);

void kmain()
{
    stack_init(kernel_stack, KERNEL_STACK_SIZE);
    do_initcalls();
    hlt();
}
