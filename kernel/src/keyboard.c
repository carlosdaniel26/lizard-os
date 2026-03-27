#include <debug.h>
#include <idt.h>
#include <init.h>
#include <io.h>
#include <keyboard.h>
#include <pic.h>
#include <setup.h>
#include <stdio.h>
#include <tty.h>
#include <types.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_PIC_MASK 0x01

int init_keyboard()
{
    PIC_unmaskIRQ(KEYBOARD_PIC_MASK);
    set_idt_gate(33, isr_keyboard, 0x8E);
    kprintf("keyboard initialized\n");
    return 0;
}

device_initcall(init_keyboard);

void isr_keyboard()
{

    while (inb(0x64) & 0x01)
    {
        u8 scancode = inb(KEYBOARD_DATA_PORT);
        tty_handler_input(scancode);
    }
    PIC_sendEOI(1);
}