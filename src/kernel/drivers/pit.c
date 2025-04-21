#include <kernel/utils/io.h>
#include <kernel/utils/alias.h>
#include <kernel/arch/ptrace.h>
#include <kernel/multitasking/task.h>
#include <kernel/arch/i686/idt.h>
#include <stdio.h>

/* PIT operates in a 1.193.182 Hz frequency*/

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_FREQUENCY_HZ 1193182
#define PIT_DESIRED_FREQUENCY_HZ PIT_FREQUENCY_HZ / 1000


void pit_init()
{
	outb(PIT_COMMAND, 0b00110110);	  /* Mode 3, Channel 0, low/high byte acess*/

	outb(PIT_CHANNEL0, PIT_DESIRED_FREQUENCY_HZ & 0xFF);	/* Low Byte */
	outb(PIT_CHANNEL0, (PIT_DESIRED_FREQUENCY_HZ >> 8));	/* High Byte */

	create_idt_descriptor(32, stub_32, 0x8E);	/* PIT */
}

extern struct pt_regs ptrace;

void isr_pit()
{	
	scheduler();
}