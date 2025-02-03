#include <stdint.h>
#include <kernel/utils/io.h>
#include <kernel/arch/idt.h>

#define PIC1_COMMAND    0x20 
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0 
#define PIC2_DATA       0xA1
#define PIC_EOI         0x20

extern void *isr_stub_table[];

/** 
 * Remap PIC vectors to avoid conflict with CPU exceptions.
 * By default, the interrupts are (0-15), while the CPU exceptions have a range between (0-31).
 * Here we remap the IRQs to 32-47.
 */
void PIC_remap() 
{
    // ICW1: Start initialization of PIC
    outb(PIC1_COMMAND, 0x11); // Master PIC
    outb(PIC2_COMMAND, 0x11); // Slave PIC

    // ICW2: Set interrupt vector offsets
    outb(PIC1_DATA, 0x20); // Master PIC vector offset (IRQ0-7)
    outb(PIC2_DATA, 0x28); // Slave PIC vector offset (IRQ8-15)

    // ICW3: Tell Master PIC there is a slave PIC at IRQ2 (0000 0100)
    outb(PIC1_DATA, 0x04); // Master PIC
    // Tell Slave PIC its cascade identity (0000 0010)
    outb(PIC2_DATA, 0x02); // Slave PIC

    // ICW4: Set PIC to x86 mode
    outb(PIC1_DATA, 0x01); // Master PIC
    outb(PIC2_DATA, 0x01); // Slave PIC

    // Unmask all interrupts on the PIC
    outb(PIC1_DATA, 0xF9); // Master PIC
    outb(PIC2_DATA, 0xFC); // Slave PIC
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
