#include <kernel/utils/alias.h>

#include <kernel/arch/idt.h>
#include <kernel/utils/io.h>
#include <kernel/terminal/tty.h>
#include <kernel/cpu/pic.h>

#define KEYBOARD_DATA_PORT 0x60

void isr_keyboard()
{
	uint8_t scancode = inb(KEYBOARD_DATA_PORT);

	tty_handler_input(scancode);
}