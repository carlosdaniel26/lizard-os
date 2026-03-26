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

__attribute__((used, section(".limine_requests"))) static volatile struct limine_executable_file_request
    executable_file_request = {.id = LIMINE_EXECUTABLE_FILE_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

u8 kernel_stack[KERNEL_STACK_SIZE];

// Refactored initcalls from kmain
__initcall(time_init_from_rtc, 1, 01);
__initcall(init_framebuffer, 1, 02);
__initcall(init_cpuid, 1, 03);
__initcall(tty_initialize, 1, 04);
__initcall(PIC_remap, 1, 05);
__initcall(init_gdt, 1, 06);
__initcall(init_idt, 1, 07);
__initcall(early_alloc_init, 1, 08);
__initcall(buddy_init, 1, 09);
__initcall(vmm_init, 1, 10);
__initcall(kmalloc_init, 1, 11);
__initcall(setup_params, 1, 12);
__initcall(task_init, 1, 13);
__initcall(syscall_init, 1, 14);
__initcall(pit_init, 1, 15);
__initcall(init_keyboard, 1, 17);
__initcall(enable_scheduler, 1, 16);

void kmain()
{
    stop_interrupts();

    asm volatile("mov %0, %%rsp" : : "r"(&kernel_stack[KERNEL_STACK_SIZE]) : "memory");

    static char k_cmdline[1024] = {0};
    strcpy(k_cmdline, executable_file_request.response->executable_file->string);

    do_initcalls();
    hlt();
}
