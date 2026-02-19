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
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>
#include <tty.h>
#include <types.h>
#include <vmm.h>

/*
 * feel dumb is temporary, the progress of commits on this
 * project is isn't so feel proud for every commit, even tho the problem is not solved.
 *
 * because its not solved yet. - Carlos, 03:46 30th December, 2025
 */

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
    framebuffer_request = {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

u8 kernel_stack[KERNEL_STACK_SIZE];

void kmain(void)
{
    asm volatile("mov %0, %%rsp" : : "r"(&kernel_stack[KERNEL_STACK_SIZE]) : "memory");

    stop_interrupts(void);
    time_init_from_rtc(void);

    if (LIMINE_BASE_REVISION_SUPPORTED == false)
    {
        hlt(void);
    }

    if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
    {
        hlt(void);
    }

    struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

    stop_interrupts(void);
    setup_framebuffer(framebuffer->width, framebuffer->height, framebuffer->address, framebuffer->pitch);
    init_cpuid(void);
    tty_initialize(void);
    pit_init(void);
    early_alloc_init(void);
    buddy_init(void);
    init_gdt(void);
    init_idt(void);
    vmm_init(void);
    kmalloc_init(void);
    task_init(void);
    syscall_init(void);
    ata_detect_devices(void);
    fat16_init(void);
    test_fat16(void); // vfs_init(void);
    PIC_remap(void);
    init_keyboard(void);
    enable_scheduler(void);
    start_interrupts(void);
    hlt(void);
}
