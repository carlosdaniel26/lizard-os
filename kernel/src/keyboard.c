#include <idt.h>
#include <io.h>
#include <keyboard.h>
#include <pic.h>
#include <stdint.h>
#include <tty.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_PIC_MASK 0x01

void init_keyboard()
{
    PIC_unmaskIRQ(KEYBOARD_PIC_MASK);
    set_idt_gate(33, isr_keyboard, 0x8E);
}

__attribute__((interrupt)) void isr_keyboard(void *frame)
{
    (void)frame;

    while (inb(0x64) & 0x01)
    {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        tty_handler_input(scancode);
    }
    PIC_sendEOI(1);
}