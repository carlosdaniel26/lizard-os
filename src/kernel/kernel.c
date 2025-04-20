#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

#include <multiboot2.h>
#include <kernel/terminal/tty.h>
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
#include <kernel/mp/mp.h>
#include <kernel/drivers/pit.h>

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
	tty_initialize();

	init_gdt();
	init_idt();
	pmm_init();
	enable_paging();
	tty_clean();
	shit_shell_init();

	start_interrupts();
	pit_init();
	init_tasks();
}