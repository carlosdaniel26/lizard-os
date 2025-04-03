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

#define IDT_ENTRIES 256

interrupt_descriptor idt[IDT_ENTRIES];
idt_ptr ptr_idt;

bool vectors[IDT_ENTRIES];

interrupt_descriptor create_idt_descriptor(void (*isr)(), uint8_t flags)
{
	interrupt_descriptor descriptor;

	descriptor.base_low  = (uint32_t)isr & 0xFFFF; /* Get just the first 16 bits*/
	descriptor.selector  = 0x08;
	descriptor.flags	 = flags;
	descriptor.base_high = ((uint32_t)isr >> 16) & 0xFFFF;
	descriptor.always0   = 0;

	return descriptor;
}

void init_idt(void)
{
	PIC_remap();

	ptr_idt.base  = (uint32_t)&idt[0];
	ptr_idt.limit = sizeof(interrupt_descriptor) * IDT_ENTRIES - 1;


	/* Set IDT descriptors*/
	idt[0] = create_idt_descriptor(stub_0,  0x8E);
	idt[6] = create_idt_descriptor(stub_6,  0x8E);
	idt[14] = create_idt_descriptor(stub_14, 0x8E);
	idt[33] = create_idt_descriptor(stub_33, 0x8E);		/* keyboard */
	/*idt[40] = create_idt_descriptor(stub_40, 0x8E);  	 timer */



	__asm__("lidt %0" : : "m"(ptr_idt));
}