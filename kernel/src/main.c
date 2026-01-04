#include <alias.h>
#include <ata.h>
#include <cpuid.h>
#include <fat16.h>
#include <framebuffer.h>
#include <gdt.h>
#include <helpers.h>
#include <idt.h>
#include <keyboard.h>
#include <limine.h>
#include <pic.h>
#include <pit.h>
#include <rtc.h>
#include <ss.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <types.h>
#include <tty.h>
#include <vmm.h>
#include <early_alloc.h>
#include <buddy.h>
#include <pgtable.h>
#include <kmalloc.h>
#include <panic.h>
#include <kernelcfg.h>
#include <ktime.h>

/* 
 * feel dumb is temporary, the progress of commits on this
 * project is isn't so feel proud for every commit, even tho the problem is not solved.
 * 
 * because its not solved yet. - Carlos, 03:46 30th December, 2025
 */

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);

__attribute__((used, section(".limine_requests"))) static volatile struct limine_framebuffer_request
	framebuffer_request
	= {.id = LIMINE_FRAMEBUFFER_REQUEST, .revision = 0};

__attribute__((used,
			   section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;

__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

u8 kernel_stack[KERNEL_STACK_SIZE];

void kmain()
{
	asm volatile("mov %0, %%rsp" : : "r"(&kernel_stack[KERNEL_STACK_SIZE]) : "memory");


	stop_interrupts();
	time_init_from_rtc();

	if (LIMINE_BASE_REVISION_SUPPORTED == false)
	{
		hlt();
	}

	if (framebuffer_request.response == NULL || framebuffer_request.response->framebuffer_count < 1)
	{
		hlt();
	}

	struct limine_framebuffer *framebuffer = framebuffer_request.response->framebuffers[0];

	stop_interrupts();
	setup_framebuffer(framebuffer->width, framebuffer->height, framebuffer->address,
					  framebuffer->pitch);
	init_cpuid();
	tty_initialize();
	pit_init();
	early_alloc_init();
	buddy_init();
	init_gdt();
	init_idt();
	vmm_init();
	kmalloc_init();
	task_init();
	ata_detect_devices();
	fat16_init();
	test_fat16();
	//vfs_init();
	PIC_remap();
	init_keyboard();
	enable_scheduler();
	hlt();
}
