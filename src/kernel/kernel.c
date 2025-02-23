#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

#include <multiboot2.h>
#include <kernel/terminal/terminal.h>
#include <kernel/arch/gdt.h>
#include <kernel/arch/idt.h>
#include <kernel/cpu/cpuid.h>
#include <kernel/cpu/pic.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/vmm.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/rtc.h>
#include <kernel/utils/io.h>
#include <kernel/utils/alias.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/multitasking/task.h>
#include <kernel/drivers/framebuffer.h>

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
	#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* just for i386 target BIN */
#if !defined(__i386__)
	#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

extern uint32_t kernel_start;
extern uint32_t kernel_end;

extern struct task *tasks;

void kernel_main(unsigned long magic_number, unsigned long addr)
{
	stop_interrupts();
	process_multiboot2_tags(magic_number, addr);
	terminal_initialize();

	init_gdt();
	init_idt();
	cpuid_get_brand();

	cpuid_kprint();
	pmm_init();

	kprintf("kernel_start: %u\nkernel_end: %u\n", &kernel_start, &kernel_end);

	enable_paging();
	kprintf("initialization finished\n");

	/*get_rtc_time();*/

	terminal_clean();
	/*kprint_rtc_time();*/
	shit_shell_init();

	asm("sti");
	/*enable_rtc_interrupts();*/

	/* tasks = pmm_alloc_block();*/

	/* create_task(tasks, &cpuid_kprint, "cpuid_kprint");*/

	/* kprint_task_state(tasks);*/
	/* terminal_clean();*/

	/*uint32_t pit_c = read_pit_count();*/

	/*kprintf("pitc: %u\n", pit_c);*/

	while(1){

	}

}