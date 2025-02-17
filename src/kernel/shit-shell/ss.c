#include <kernel/terminal/terminal.h>

extern uint8_t input_column_start;
extern uint8_t input_row_start;

void kprint_prompt()
{
	terminal_writestring("root: ");
	input_column_start = terminal_get_column();
	input_row_start = terminal_get_row();
}

void shit_shell_init()
{
	kprint_prompt();
	terminal_enable_cursor(input_column_start-1, input_row_start);
}