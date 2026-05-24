#include <lizard/debug.h>
#include <lizard/idt.h>
#include <lizard/init.h>
#include <lizard/io.h>
#include <lizard/keyboard.h>
#include <lizard/pic.h>
#include <lizard/setup.h>
#include <nolibc/stdio.h>
#include <lizard/tty.h>
#include <nolibc/types.h>

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_VECTOR 33

int init_keyboard()
{
    PIC_unmaskVector(KEYBOARD_VECTOR);
    isr_table[KEYBOARD_VECTOR] = &isr_keyboard;
    kprintf("keyboard initialized\n");
    return 0;
}

device_initcall(init_keyboard);

void isr_keyboard(struct cpu_state *regs)
{
    (regs);
    while (inb(0x64) & 0x01)
    {
        u8 scancode = inb(KEYBOARD_DATA_PORT);
        tty_handler_input(scancode);
    }
    PIC_sendEOI(1);
}