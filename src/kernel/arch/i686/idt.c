#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <kernel/utils/alias.h>
#include <kernel/arch/idt.h>
#include <kernel/terminal/tty.h>
#include <kernel/terminal/vga.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/utils/io.h>
#include <kernel/cpu/isr.h>
#include <kernel/cpu/pic.h>
#include <kernel/drivers/pit.h>

#define IDT_ENTRIES 256

interrupt_descriptor idt[IDT_ENTRIES];
idt_ptr ptr_idt;

bool vectors[IDT_ENTRIES];

void create_idt_descriptor(unsigned int_id, void (*isr)(), uint8_t flags)
{
	interrupt_descriptor descriptor;

	descriptor.base_low  = (uint32_t)isr & 0xFFFF; /* Get just the first 16 bits*/
	descriptor.selector  = 0x08;
	descriptor.flags	 = flags;
	descriptor.base_high = ((uint32_t)isr >> 16) & 0xFFFF;
	descriptor.always0   = 0;

	idt[int_id] = descriptor;
}

void init_idt(void)
{
	PIC_remap();

	ptr_idt.base  = (uint32_t)&idt[0];
	ptr_idt.limit = sizeof(interrupt_descriptor) * IDT_ENTRIES - 1;


	/* Set IDT descriptors*/
	create_idt_descriptor(0, stub_0,  0x8E);
	create_idt_descriptor(16, stub_6,  0x8E);
	create_idt_descriptor(14, stub_14, 0x8E);
	create_idt_descriptor(33, stub_33, 0x8E);		/* Keyboard */



	__asm__("lidt %0" : : "m"(ptr_idt));
}