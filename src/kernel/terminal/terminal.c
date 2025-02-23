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

size_t terminal_row;
size_t terminal_column;
uint32_t terminal_color;
uint32_t terminal_background_color;
uint8_t terminal_color_scheme;
uint16_t* terminal_buffer;

uint32_t input_column_start;
uint32_t input_row_start;

void terminal_initialize(void)
{
	extern uint32_t width;
	extern uint32_t height;

	terminal_width = width;
	terminal_height = height;

	terminal_row = 0;
	terminal_column = 0;
	terminal_set_background_color(VGA_COLOR_BLACK);
	terminal_setcolor(VGA_COLOR_WHITE);
}

void terminal_set_column(int index)
{
	terminal_column = index;
}

void terminal_set_row(int index)
{
	terminal_row = index;
}

int terminal_get_column()
{
	return terminal_column;
}

int terminal_get_row()
{
	return terminal_row;
}

void terminal_clean(void)
{
	terminal_set_row(0);
	terminal_set_column(0);
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
	draw_char(x * 8, y, color, c);
}

void terminal_putchar(char c)
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		terminal_row++;
	}

	if (terminal_row == VGA_HEIGHT) {
		terminal_row = VGA_HEIGHT - 1; /* Reset to the last row*/
	}


	terminal_update_cursor();
}

void terminal_breakline(void)
{
	terminal_row++;
	terminal_column = 0;
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


#define VGA_CTRL 0x3D4
#define VGA_DATA 0x3D5

void set_cursor_style(uint8_t start_line, uint8_t end_line) {
	/* Envia o índice do registrador de início*/
	outb(VGA_CTRL, 0x0A);
	outb(VGA_DATA, start_line);

	/* Envia o índice do registrador de fim*/
	outb(VGA_CTRL, 0x0B);
	outb(VGA_DATA, end_line);
}

void terminal_disable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void terminal_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
	set_cursor_style(0, 15);
}


void terminal_update_cursor()
{
	uint16_t pos = terminal_row * VGA_WIDTH + (terminal_column);

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
}

void terminal_backspace()
{
	terminal_set_column(terminal_column - 1);
	terminal_putchar(' ');
	terminal_set_column(terminal_column - 1);
	terminal_update_cursor();
}

void terminal_handler_input(char scancode)
{
	char key = scancode;

	if (key == KEY_BACKSAPCE)
	{
		if (terminal_row == input_row_start && input_column_start < terminal_column)
		{
			terminal_backspace();
		}
		else if (terminal_row > input_row_start)
		{
			if (terminal_column == 0)
			{
				/* Come back to upper line if go to the left limit*/
				terminal_row--;
				terminal_column = VGA_WIDTH;
			}

			/* If came to the start of the input, stop*/
			if (terminal_row == input_row_start && terminal_column < input_column_start)
			{
				terminal_column = input_column_start;
			}
			else
			{
				terminal_backspace();
			}
		}
	}


	else if (key == KEY_ENTER)
	{
		int row = terminal_get_row();

		terminal_set_row(row+1);
		terminal_set_column(0);
		kprint_prompt();
		terminal_update_cursor();
	}
	else
	{
		if ((unsigned)scancode < 0x80) /* dont handle break codes (scancode >= 0x80)*/
		{
			key = convertScancode[(unsigned)scancode];

			terminal_putentryat(key, terminal_color, ++input_column_start, input_row_start);

			if (input_column_start == terminal_width)
			{
				input_column_start = 0;
				input_row_start++;
				kprintf("OP");
			}
		}
	}
}