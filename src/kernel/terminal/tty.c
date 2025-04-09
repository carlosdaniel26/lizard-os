#include <kernel/terminal/tty.h>
#include <kernel/terminal/vga.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/utils/io.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/framebuffer.h>

#include <string.h>
#include <stdbool.h>
#include <stdio.h>

size_t terminal_width;
size_t terminal_height;

size_t terminal_text_width;
size_t terminal_text_height;

size_t terminal_row;
size_t terminal_column;
uint32_t terminal_color;
uint32_t terminal_background_color;

size_t cmd_start_column;
size_t cmd_start_row;

void tty_initialize()
{
	extern uint32_t width;
	extern uint32_t height;

	terminal_width = width;
	terminal_height = height;


	terminal_text_width = width / FONT_WIDTH;
	terminal_text_height = height / FONT_HEIGHT;

	terminal_row = 0;
	terminal_column = 0;
	terminal_background_color = VGA_COLOR_WHITE;
	terminal_color = VGA_COLOR_BLUE;
}

static inline bool is_pos_after_input(unsigned row, unsigned col)
{
	if (row > cmd_start_row)
		return true;

	if (row == cmd_start_row && col > cmd_start_column)
		return true;

	return false;
}

static inline bool is_cursor_after_input()
{
	return is_pos_after_input(terminal_row, terminal_column);
}

void tty_clean()
{
	terminal_row = 0;
	terminal_column = 0;
	clear_framebuffer();

}

void tty_putentryat(char c, uint32_t color, size_t x, size_t y)
{
	draw_char(x * FONT_WIDTH, y * FONT_HEIGHT, color, c);
}

void tty_putchar(char c)
{
	tty_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == terminal_text_width)
	{
		tty_breakline();
	}
}

void tty_breakline()
{
	terminal_row++;
	terminal_column = 0;
}

static inline void tty_tab()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		tty_putchar(' ');
		

		if (terminal_column == terminal_width)
		{
			terminal_column = 0;
			terminal_row++;
		}
	}
}

static inline bool is_ascii_character(char c)
{
	if (c >= ' ' && c <= '~')
		return true;

	return false;
}

void tty_write(const char* data, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		if (data[i] == '\n')
		{
			tty_breakline();
			i+=1;
		}

		else
		{
			tty_putchar(data[i]);
		}
	}
}

void tty_writestring(const char* data)
{
	tty_write(data, strlen(data));
}

void tty_backspace()
{
	if (is_cursor_after_input())
	{
		tty_putentryat(' ', terminal_color, terminal_column-1, terminal_row);
		terminal_column--;

		if (terminal_column == 0)
		{
			terminal_column = terminal_text_width;
			terminal_row--; 
		}
	}
}

void tty_handler_input(char scancode)
{

	if (scancode == KEY_BACKSAPCE)
	{
		tty_backspace();
	}

	else if (scancode == KEY_ENTER)
	{
		terminal_row += 1;
		terminal_column = 0;
		kprint_prompt();
	}
	else if ((unsigned)scancode < 0x80) /* dont handle break codes (scancode >= 0x80)*/
	{
		char c = convertScancode[(unsigned)scancode];

		if (c == '\t')
		{
			tty_tab();
		}
		else if (is_ascii_character(c))
		{
			tty_putchar(c);

			if (terminal_column == terminal_text_width)
			{
				tty_breakline();
			}
		}
	}
}