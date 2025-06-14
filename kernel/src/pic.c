#include <stdint.h>
#include <io.h>
#include <idt.h>

#define PIC1_COMMAND        0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND        0xA0
#define PIC2_DATA       0xA1
#define PIC_EOI         0x20

#define PIC_INIT_COMMAND    0x11
#define PIC_CASCADE_CONFIG  0x04
#define PIC_MODE_CONFIG     0x01

#define PIC_VECTOR_OFFSET1 32
#define PIC_VECTOR_OFFSET2 40

#define SLAVE_PIC_HAS_TO_BE_WARNED irq >= 8

/**
 * Remap PIC vectors to avoid conflict with CPU exceptions.
 * By default, the interrupts are (0-15), while the CPU exceptions have a range between (0-31).
 * Here we remap the IRQs to 32-47.
 */
void PIC_remap()
{
    /* Start initialization of PIC */
    outb(PIC1_COMMAND, PIC_INIT_COMMAND); /* Master PIC*/
    outb(PIC2_COMMAND, PIC_INIT_COMMAND); /* Slave PIC*/

    /* Interrupt vector offsets */
    outb(PIC1_DATA, PIC_VECTOR_OFFSET1); /* (IRQ0-7) -> (32-39)*/
    outb(PIC2_DATA, PIC_VECTOR_OFFSET2); /* (IRQ8-15) -> (40-47)*/

    /* Tell Master PIC there is a slave PIC at IRQ2 (0000 0100) */
    outb(PIC1_DATA, PIC_CASCADE_CONFIG); /* Master PIC*/

    /* Tell Slave PIC its cascade identity (0000 0010) */
    outb(PIC2_DATA, 0x02);

    /* Set PIC to x86 mode */
    outb(PIC1_DATA, PIC_MODE_CONFIG);
    outb(PIC2_DATA, PIC_MODE_CONFIG);

    /* Unmask interrupts on the PIC */
    outb(PIC1_DATA, 0xF9);
    outb(PIC2_DATA, 0xFC);
}

void PIC_sendEOI(uint8_t irq)
{
    if (SLAVE_PIC_HAS_TO_BE_WARNED)
    {
        outb(PIC2_COMMAND, PIC_EOI);
    }

    outb(PIC1_COMMAND, PIC_EOI);
}

void PIC_unmaskIRQ(uint8_t irq) 
{
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port);
    value &= ~(1 << irq);
    outb(value, port);
}