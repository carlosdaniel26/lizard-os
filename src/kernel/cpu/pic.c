#include <stdint.h>
#include <kernel/utils/io.h>
#include <kernel/arch/idt.h>

#define PIC1_COMMAND    	0x20
#define PIC1_DATA       	0x21
#define PIC2_COMMAND    	0xA0
#define PIC2_DATA       	0xA1
#define PIC_EOI         	0x20

#define PIC_INIT_COMMAND 	0x11
#define PIC_CASCADE_CONFIG 	0x04
#define PIC_MODE_CONFIG 	0x01

#define PIC1_OFFSET	32
#define PIC2_OFFSET	40

extern void *isr_stub_table[];

/** 
 * Remap PIC vectors to avoid conflict with CPU exceptions.
 * By default, the interrupts are (0-15), while the CPU exceptions have a range between (0-31).
 * Here we remap the IRQs to 32-47.
 */

void PIC_remap() 
{
	// Start initialization of PIC
	outb(PIC1_COMMAND, PIC_INIT_COMMAND);
	outb(PIC2_COMMAND, PIC_INIT_COMMAND);

	/* interrupt vector offsets */
	outb(PIC1_DATA, PIC1_OFFSET); // (IRQ0-7) -> (IRQ 32-39)
	outb(PIC2_DATA, PIC2_OFFSET); // Slave PIC vector offset (IRQ 40-47)

	/* Tell Master PIC there is a slave PIC at IRQ2 (0000 0100) */
	outb(PIC1_DATA, PIC_CASCADE_CONFIG); // Master PIC
	
	/* Tell Slave PIC its cascade identity (0000 0010) */
	outb(PIC2_DATA, 0x02); // Slave PIC

	/* Set PIC to x86 mode */
	outb(PIC_MODE_CONFIG, 0x01); // Master PIC
	outb(PIC_MODE_CONFIG, 0x01); // Slave PIC

	/* Unmask all interrupts on the PIC */
	outb(PIC1_DATA, 0xF9);
	outb(PIC2_DATA, 0xFC);
}

void PIC_sendEOI(uint8_t irq)
{
	// The Slave PIC has to be warned?
	if (irq >= 8) {
		outb(PIC2_COMMAND, PIC_EOI);
	}

	// Warn the Master PIC
	outb(PIC1_COMMAND, PIC_EOI);
}
