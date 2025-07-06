#include <io.h>
#include <alias.h>
#include <task.h>
#include <idt.h>
#include <stdio.h>
#include <pit.h>
#include <pic.h>

/* PIT operates in a 1.193.182 Hz frequency*/

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_FREQUENCY_HZ 1193182

#define PIT_TARGET_HZ 100
#define PIT_DESIRED_FREQUENCY_HZ (PIT_FREQUENCY_HZ / PIT_TARGET_HZ)

static inline void pit_mask()
{
    #define PIC1_DATA 0x21
    uint8_t mask = inb(PIC1_DATA);
    mask |= 0x01;
    outb(PIC1_DATA, mask);
}

static inline void pit_unmask()
{
    #define PIC1_DATA 0x21
    uint8_t mask = inb(PIC1_DATA);
    mask |= 0x01;
    mask ^= 0x01;
    outb(PIC1_DATA, mask);
}

void pit_init()
{
    outb(PIT_COMMAND, 0b00110110);    /* Mode 3, Channel 0, low/high byte acess*/

    outb(PIT_CHANNEL0, PIT_DESIRED_FREQUENCY_HZ & 0xFF);    /* Low Byte */
    outb(PIT_CHANNEL0, (PIT_DESIRED_FREQUENCY_HZ >> 8));    /* High Byte */

    set_idt_gate(32, isr_pit, 0x8E);   /* PIT */
    pit_unmask();
}

__attribute__((interrupt))
void isr_pit(InterruptFrame *frame)
{
    (void)frame;
    //scheduler();

    extern void hlt();
    frame->rip = (uint64_t)&hlt;
    PIC_sendEOI(32);
}