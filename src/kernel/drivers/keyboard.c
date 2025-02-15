#include <kernel/utils/alias.h>

#include <kernel/arch/idt.h>
#include <kernel/utils/io.h>
#include <kernel/terminal/terminal.h>
#include <kernel/cpu/pic.h>

#define KEYBOARD_DATA_PORT 0x60

void isr_keyboard()
{
	uint8_t scancode = inb(KEYBOARD_DATA_PORT);

	terminal_handler_input(scancode);
}