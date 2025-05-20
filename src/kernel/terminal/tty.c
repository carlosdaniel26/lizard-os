#include <kernel/terminal/tty.h>
#include <kernel/terminal/vga.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/utils/io.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/framebuffer.h>
#include <kernel/mem/kmalloc.h>

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

char *text_buffer;

extern uint32_t *fb;

void tty_initialize()
{
	extern uint32_t width;
	extern uint32_t height;

	terminal_width = width;
	terminal_height = height;

	terminal_text_width = width / FONT_WIDTH;
	terminal_text_height = height / FONT_HEIGHT;

	text_buffer = kmalloc(terminal_text_width * terminal_text_height);

	terminal_row = 0;
	terminal_column = 0;
	terminal_background_color = VGA_COLOR_BLACK;
	terminal_color = VGA_COLOR_WHITE;
}

void tty_scroll()
{
    size_t char_rows = terminal_text_height;
    size_t char_cols = terminal_text_width;

    for (size_t row = 1; row < char_rows; row++)
    {
        for (size_t col = 0; col < char_cols; col++)
        {
            // Copia caractere da linha de baixo pra cima
            // A função draw_char precisa saber (em pixels): col * FONT_WIDTH, row * FONT_HEIGHT
            // Então isso só funciona se você armazenar os caracteres em um buffer
            // MAS como você tá desenhando diretamente, vamos copiar do framebuffer mesmo

            // Calcula a posição em pixels da origem
            uint32_t* src = (uint32_t*)fb + ((row * FONT_HEIGHT) * terminal_width);
            uint32_t* dst = (uint32_t*)fb + (((row - 1) * FONT_HEIGHT) * terminal_width);

            // Copia uma "linha de caractere" inteira (em pixels)
            for (size_t y = 0; y < FONT_HEIGHT; y++) {
                memcpy(dst + y * terminal_width, src + y * terminal_width, terminal_width * 4);
            }
        }
    }

    // Limpa a última linha
    for (size_t y = (char_rows - 1) * FONT_HEIGHT; y < char_rows * FONT_HEIGHT; y++)
    {
        uint32_t* line = (uint32_t*)fb + y * terminal_width;
        for (size_t x = 0; x < terminal_width; x++)
        {
            line[x] = terminal_background_color;
        }
    }

    terminal_row--;
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
	text_buffer[(y * terminal_text_width) +  x] = c;
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
	if (terminal_row >= terminal_text_height)
	{
		tty_scroll();
	}
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
		char cmd_buffer[512];

		size_t i = 0;
		size_t j = 0;

		for (i = (cmd_start_row * terminal_text_width) + cmd_start_column; 
			i < (terminal_row * terminal_text_width) + terminal_column;
			i++)
		{

			cmd_buffer[j++] = text_buffer[i];
		}

		cmd_buffer[j] = '\0';

		tty_breakline();
		runcmd(cmd_buffer);
		
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