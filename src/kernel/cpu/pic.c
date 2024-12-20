#include <stdint.h>

#include <kernel/utils/io.h>
#include <kernel/arch/idt.h>

#define PIC1_COMMAND	0x20 
#define PIC2_COMMAND	0xA0 
#define PIC_EOI			0x20

extern void *isr_stub_table[];

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
    if (irq >= 8) {
        // Se a IRQ for >= 8, significa que o PIC escravo foi acionado
        outb(PIC2_COMMAND, PIC_EOI); // Enviar EOI para o PIC escravo
    }

    // Sempre enviar EOI para o PIC mestre
    outb(PIC1_COMMAND, PIC_EOI); // Enviar EOI para o PIC mestre
}

// IRQs

void init_irq() 
{    
    outb(0x21, ~(1 << 1));
    outb(0x21, 0xFD); // Habilita apenas IRQ1 (0b11111101)
    asm volatile("sti");
}