#include <kernel/terminal/terminal.h>
#include <kernel/terminal/vga.h>

extern size_t cmd_start_column;
extern size_t cmd_start_row;
extern size_t terminal_row;
extern size_t terminal_column;

void kprint_prompt()
{
	terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
	terminal_writestring("root: ");
	terminal_setcolor(VGA_COLOR_WHITE);
	cmd_start_column = terminal_column;
	cmd_start_row = terminal_row;
}

void shit_shell_init()
{
	kprint_prompt();
}