#include <framebuffer.h>
#include <spinlock.h>
#include <ss.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <tty.h>
#include <vga.h>

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

char text_buffer[1000 * 1000];

uint32_t *fb;

static spinlock_t tty_lock;

void tty_initialize()
{
    extern uint32_t width;
    extern uint32_t height;
    extern uint32_t *framebuffer;

    terminal_width = width;
    terminal_height = height;
    fb = framebuffer;

    terminal_text_width = width / FONT_WIDTH;
    terminal_text_height = height / FONT_HEIGHT;

    terminal_row = 0;
    terminal_column = 0;
    terminal_background_color = TERMINAL_BG_COLOR;
    terminal_color = TERMINAL_COLOR;

    spinlock_init(&tty_lock);
}

void tty_scroll()
{
    scroll_framebuffer(FONT_HEIGHT);
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
    text_buffer[(y * terminal_text_width) + x] = c;
}

char tty_putchar(char c)
{
    spinlock_acquire(&tty_lock);
    if (c == '\n') {
        tty_breakline();
        spinlock_release(&tty_lock);
        return c;
    } else if (c == '\t') {
        spinlock_release(&tty_lock);
        tty_tab();
        return c;
    }

    tty_putentryat(c, terminal_color, terminal_column, terminal_row);

    if (++terminal_column == terminal_text_width) {
        tty_breakline();
    }

    spinlock_release(&tty_lock);

    return c;
}

void tty_breakline()
{
    terminal_row++;
    terminal_column = 0;
    if (terminal_row == terminal_text_height) {
        tty_scroll();
    }
}

#define TAB_SIZE 4

void tty_tab()
{
    for (uint8_t i = terminal_column; i % 4 == TAB_SIZE; i++) {
        tty_putchar(' ');

        if (terminal_column == terminal_width) {
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

void tty_write(const char *data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        tty_putchar(data[i]);
    }
    // spinlock_release(&tty_lock);
}

void tty_writestring(const char *data)
{
    tty_write(data, strlen(data));
}

void tty_backspace()
{
    if (is_cursor_after_input()) {
        tty_putentryat(' ', terminal_color, terminal_column - 1, terminal_row);
        terminal_column--;

        if (terminal_column == 0) {
            terminal_column = terminal_text_width;
            terminal_row--;
        }
    }
}

#define KEY_BACKSAPCE 0x0E
#define KEY_ENTER 0x1C

void tty_handler_input(char scancode)
{

    if (scancode == KEY_BACKSAPCE) {
        tty_backspace();
    }

    else if (scancode == KEY_ENTER) {
        char cmd_buffer[512];

        size_t i = 0;
        size_t j = 0;

        for (i = (cmd_start_row * terminal_text_width) + cmd_start_column;
             i < (terminal_row * terminal_text_width) + terminal_column; i++) {

            cmd_buffer[j++] = text_buffer[i];
        }

        cmd_buffer[j] = '\0';

        tty_breakline();
        runcmd(cmd_buffer);

        kprint_prompt();
    } else if ((unsigned)scancode < 0x80) /* dont handle break codes (scancode >= 0x80)*/
    {
        char c = convertScancode[(unsigned)scancode];

        if (is_ascii_character(c)) {
            tty_putchar(c);

            if (terminal_column == terminal_text_width) {
                tty_breakline();
            }
        }
    }
}