#include <kernel/terminal/terminal.h>
#include <kernel/terminal/vga.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/utils/io.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/framebuffer.h>

#include <string.h>
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

void terminal_initialize()
{
	extern uint32_t width;
	extern uint32_t height;

	terminal_width = width;
	terminal_height = height;


	terminal_text_width = width / FONT_WIDTH;
	terminal_text_height = height / FONT_HEIGHT;

	terminal_row = 0;
	terminal_column = 0;
	terminal_set_background_color(VGA_COLOR_BLACK);
	terminal_setcolor(VGA_COLOR_WHITE);
}

static inline bool is_cursor_after_input()
{
	if (terminal_row > cmd_start_row)
		return true;

	if (terminal_row == cmd_start_row && terminal_column > cmd_start_column)
		return true;

	return false;
}

void terminal_clean()
{
	terminal_row = 0;
	terminal_column = 0;
	clear_framebuffer();

}

void terminal_setcolor(uint32_t color)
{
	terminal_color = color;
}

void terminal_set_background_color(uint32_t background_color)
{
	terminal_background_color = background_color;
}

void terminal_putentryat(char c, uint32_t color, size_t x, size_t y)
{
	draw_char(x * FONT_WIDTH, y * FONT_HEIGHT, color, c);
}

void terminal_putchar(char c)
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == terminal_text_width)
	{
		terminal_breakline();
	}
}

void terminal_breakline()
{
	terminal_row++;
	terminal_column = 0;
}

static inline void terminal_tab()
{
	for (uint8_t i = 0; i < 4; i++)
	{
		terminal_putchar(' ');
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

void terminal_write(const char* data, size_t size)
{
	for (size_t i = 0; i < size; i++)
	{
		if (data[i] == '\n')
		{
			terminal_breakline();
			i+=1;
		}

		else
		{
			terminal_putchar(data[i]);
		}
	}
}

void terminal_writestring(const char* data)
{
	terminal_write(data, strlen(data));
}

void terminal_backspace()
{
	if (is_cursor_after_input())
	{
		terminal_putchar(' ');
		terminal_column--;
	}
}

void terminal_handler_input(char scancode)
{

	if (scancode == KEY_BACKSAPCE)
	{
		terminal_backspace();
	}

	else if (scancode == KEY_ENTER)
	{
		int row = terminal_row;
		terminal_row = row + 1;
		terminal_column = 0;
		kprint_prompt();
	}
	else if ((unsigned)scancode < 0x80) /* dont handle break codes (scancode >= 0x80)*/
	{
		char c = convertScancode[(unsigned)scancode];

		if (c == '\t')
		{
			terminal_tab();
		}
		else if (is_ascii_character(c))
		{
			terminal_putchar(c);

			if (terminal_column == terminal_text_width)
			{
				terminal_breakline();
			}
		}
	}
}