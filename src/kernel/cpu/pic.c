#include <stdint.h>

#include <kernel/utils/io.h>
#include <kernel/arch/idt.h>

#define PIC1_COMMAND	0x20 
#define PIC2_COMMAND	0xA0 
#define PIC_EOI			0x20

extern void *isr_stub_table[];


/** 
 * Remap PIC vectors to avoid conflit with cpu exceptions
 * By default the interrupts is (0-15), while the cpu exceptions has a range beetween (0-31)
 * Then here we remap the IRQs to 32-47
 */

void PIC_remap() 
{
    // ICW1: Start initialization of PIC
    outb(0x20, 0x11); // Master PIC
    outb(0xA0, 0x11); // Slave PIC

    // ICW2: Set interrupt vector offsets
    outb(0x21, 0x20); // Master PIC vector offset (IRQ0-7)
    outb(0xA1, 0x28); // Slave PIC vector offset (IRQ8-15)

    // ICW3: Tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    outb(0x21, 0x04); // Master PIC
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(0xA1, 0x02); // Slave PIC

    // ICW4: Set PIC to x86 mode
    outb(0x21, 0x01); // Master PIC
    outb(0xA1, 0x01); // Slave PIC

    // Mask all interrupts on the PIC
    outb(0x21, 0xFF); // Master PIC
    outb(0xA1, 0xFF); // Slave PIC
}

void PIC_sendEOI(uint8_t irq)
{
    /* The Slave Pic has To Be Warned?*/
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    /* Warn The Master Pic*/
    outb(PIC1_COMMAND, PIC_EOI);
}

// IRQs

void init_irq() 
{    
    outb(0x21, ~(1 << 1));
    outb(0x21, 0xFC);
    asm volatile("sti");
}