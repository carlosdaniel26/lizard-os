#include <lizard/helpers.h>
#include <lizard/panic.h>
#include <nolibc/stdarg.h>
#include <nolibc/stdio.h>
#include <lizard/tty.h>
#include <nolibc/types.h>

void kpanic(const char *str, ...)
{
    extern u32 tty_color;
    u32 saved = tty_color;

    tty_color = VGA_COLOR_RED;
    kprintf("\nKERNEL PANIC: ");

    va_list args;
    va_start(args, str);
    kvprintf(str, args); /* not kprintf(str, args) - args is a va_list */
    va_end(args);

    kprintf("\n");
    tty_color = saved;

    stop_cpu();
}
