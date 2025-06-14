#include <stdint.h>
#include <tty.h>
#include <io.h>
#include <pic.h>

#define KEYBOARD_DATA_PORT 0x60

__attribute__((interrupt))
void isr_keyboard(void *frame)
{
 	while (inb(0x64) & 0x01)
    {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        tty_handler_input(scancode);
    }
	PIC_sendEOI(1);
}