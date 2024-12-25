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

int kernel_main(uint32_t magic_number, struct multiboot_info_t* mb_info) 
{
	terminal_initialize();
	
	if (magic_number == 0x36D76289)
	{
		if (mb_info->magic_number == 0x36D76289)
		{
			printf("multiboot2 loaded\n");
		}
		else
		{
			printf("multiboot2 error on load\nmb_info->magic_number: %u\n", mb_info->magic_number);
		}
	}
	else
		printf("multiboot2 error on load\n");


	init_gdt();
	init_idt();
	init_irq();
	cpuid_get_brand();

	cpuid_print();
	//pmm_init(mb_info);
	
	printf("kernel_start: %u\n", &kernel_start);

	process_multiboot_tags(mb_info);

	//init_paging();

	printf("initialization finished\n");

	shit_shell_init();

	while(1) {
		
	}

	return 1;
	
}