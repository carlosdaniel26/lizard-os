#include <stdio.h>
#include <tty.h>
#include <stdarg.h>
#include <types.h>
#include <panic.h>
#include <helpers.h>

void kpanic(const char *str, ...)
{
	extern u32 tty_color;

	u32 temp = tty_color;

	tty_color = VGA_COLOR_RED;

	va_list args;
	va_start(args, str);

	kprintf("KERNEL PANIC: ");
	tty_color = temp;

	kprintf(str, args);

	hlt();
}