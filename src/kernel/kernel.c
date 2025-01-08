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
#include <kernel/utils/io.h>
#include <kernel/shit-shell/ss.h>

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
extern uint32_t kernel_size;

void kernel_main(unsigned long magic_number, unsigned long addr) 
{
	terminal_initialize();
	process_multiboot2_tags(magic_number, addr);

	init_gdt();
	init_idt();
	init_irq();
	cpuid_get_brand();

	cpuid_print();
	pmm_init();
	
	printf("kernel_start: %u\nkernel_end: %u\n", &kernel_start, &kernel_end);

	extern void enable_paging();

	printf("initialization finished\n");

	enable_paging();
	shit_shell_init();

	while(1) {
		
	}
	
}