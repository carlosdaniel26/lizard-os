#include <stdint.h>
#include <io.h>

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

#define ICW1_INIT    0x10
#define ICW1_ICW4    0x01
#define ICW4_8086    0x01
#define PIC_EOI      0x20

void pic_remap(int offset1, int offset2) 
{
    uint8_t a1, a2;

    /* Save previous mask */
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);

    /* Init on ICW4 mode */
    outb(ICW1_INIT | ICW1_ICW4, PIC1_COMMAND);
    io_wait();
    outb(ICW1_INIT | ICW1_ICW4, PIC2_COMMAND);
    io_wait();

    /* Offsets */
    outb(offset1, PIC1_DATA);
    io_wait();
    outb(offset2, PIC2_DATA);
    io_wait();

    /* Setup Master/Slave */
    outb(4, PIC1_DATA);
    io_wait();
    outb(2, PIC2_DATA);
    io_wait();

    /* 8086 Mode */
    outb(ICW4_8086, PIC1_DATA);
    io_wait();
    outb(ICW4_8086, PIC2_DATA);
    io_wait();

    /* Restore previous mask */
    outb(a1, PIC1_DATA);
    outb(a2, PIC2_DATA);
}

void pic_unmask_irq(uint8_t irq) 
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

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) 
    {
        outb(PIC_EOI, PIC2_COMMAND);
    }
    outb(PIC_EOI, PIC1_COMMAND);
}
