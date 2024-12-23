#include <kernel/terminal/terminal.h>

extern uint8_t input_column_start;
extern uint8_t input_row_start;

void print_prompt()
{
	terminal_writestring("root: ");
	input_column_start = terminal_get_column();
	input_row_start = terminal_get_row();
}

void shit_shell_init()
{
	print_prompt();
}