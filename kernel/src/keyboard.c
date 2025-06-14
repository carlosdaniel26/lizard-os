#include <stdint.h>
#include <tty.h>
#include <io.h>

#define KEYBOARD_DATA_PORT 0x60

void isr_keyboard()
{
 	while (inb(0x64) & 0x01)
    {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        tty_handler_input(scancode);
    }
	PIC_sendEOI(1);
}