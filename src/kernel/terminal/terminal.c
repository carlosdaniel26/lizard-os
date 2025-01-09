#include <kernel/terminal/terminal.h>
#include <kernel/terminal/vga.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/utils/io.h>

#include <string.h>

#define VIDEO_BUFFER_START 0xB8000

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint8_t terminal_background_color;
uint8_t terminal_color_scheme;
uint16_t* terminal_buffer;

uint8_t input_column_start;
uint8_t input_row_start;

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_set_background_color(VGA_COLOR_WHITE);
	terminal_setcolor(VGA_COLOR_BLACK);

	terminal_initialize_buffer();
	terminal_initialize_background();
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

void terminal_initialize_background(void)	
{
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color_scheme);
		}
	}
}

void terminal_clean(void)
{
	terminal_initialize_background();
	terminal_set_row(0);
	terminal_set_column(0);
	
}

void terminal_initialize_buffer(void)
{
	terminal_buffer = (uint16_t*) VIDEO_BUFFER_START;
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
	terminal_update_color_scheme();
}

void terminal_set_background_color(uint8_t background_color)
{
	terminal_background_color = background_color;
	terminal_update_color_scheme();
}

void terminal_update_color_scheme(void)
{
	terminal_color_scheme = vga_entry_color(terminal_color, terminal_background_color);
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
	terminal_putentryat(c, terminal_color_scheme, terminal_column, terminal_row);
	if (++terminal_column == VGA_WIDTH) {
		terminal_column = 0;
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
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
	if (terminal_row < input_row_start) return;
	
	if (terminal_column > input_column_start)
	{
		terminal_set_column(terminal_column - 1);
		terminal_putchar(' ');
		terminal_set_column(terminal_column - 1);
	}
	terminal_update_cursor();
}

void terminal_handler_input(char scancode)
{
	char key = scancode;

	if (key == 0x0E)
	{
		terminal_backspace();
	}
	else if (key == 0x1C)
	{
		int row = terminal_get_row();

		terminal_set_row(row+1);
		terminal_set_column(0);
	}
	else
	{
		if ((unsigned)scancode < 0x80) // dont handle break codes (< 0x80)
		{
			key = convertScancode[(unsigned)scancode];
	
			terminal_putchar(key);
		}
	}
}