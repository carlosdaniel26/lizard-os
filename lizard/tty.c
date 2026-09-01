#include <lizard/framebuffer.h>
#include <lizard/init.h>
#include <lizard/io.h>
#include <lizard/spinlock.h>
#include <lizard/ss.h>
#include <nolibc/stdbool.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <lizard/tty.h>

#include <lizard/vga.h>

size_t terminal_width;
size_t terminal_height;

size_t terminal_text_width;
size_t terminal_text_height;

size_t terminal_row;
size_t terminal_column;
u32 tty_color;
u32 tty_bg_color;

size_t cmd_start_column;
size_t cmd_start_row;

char text_buffer[1000 * 1000];

u32 *fb;

static struct spinlock_t tty_lock;

static int tty_init()
{
    fb = framebuffer;

    if (framebuffer)
    {
        terminal_width = width;
        terminal_height = height;
        terminal_text_width = width / FONT_WIDTH;
        terminal_text_height = height / FONT_HEIGHT;
    }
    else
    {
        /* Serial-only console: keep a sane virtual grid so the cursor /
         * text_buffer bookkeeping stays in bounds. Real output goes to COM1
         * via serial_putc(). */
        terminal_text_width = 80;
        terminal_text_height = 25;
        terminal_width = terminal_text_width * FONT_WIDTH;
        terminal_height = terminal_text_height * FONT_HEIGHT;
    }

    terminal_row = 0;
    terminal_column = 0;
    tty_bg_color = TTY_DEFAULT_BG_COLOR;
    tty_color = TTY_DEFAULT_COLOR;

    spinlock_init(&tty_lock);

    return 0;
}

early_initcall(tty_init);

void tty_scroll()
{
    scroll_framebuffer(FONT_HEIGHT);

    terminal_row--;
}

static inline bool is_pos_after_input(unsigned row, unsigned col)
{
    if (row > cmd_start_row) return true;

    if (row == cmd_start_row && col > cmd_start_column) return true;

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

void tty_putentryat(char c, u32 color, size_t x, size_t y)
{
    draw_char(x * FONT_WIDTH, y * FONT_HEIGHT, color, c);
    text_buffer[(y * terminal_text_width) + x] = c;
}

/* Best-effort COM1 mirror of console output; bounded so a missing UART can't hang. */
static void serial_putc(char c)
{
    for (int i = 0; i < 100000 && !(inb(0x3f8 + 5) & 0x20); i++)
        ;
    outb(0x3f8, (u8)c);
}

char tty_putchar(char c)
{
    if (c == '\n') serial_putc('\r');
    serial_putc(c);

    spinlock_lock(&tty_lock);
    if (c == '\n')
    {
        tty_breakline();
        spinlock_unlock(&tty_lock);
        return c;
    }
    else if (c == '\t')
    {
        spinlock_unlock(&tty_lock);
        tty_tab();
        return c;
    }

    tty_putentryat(c, tty_color, terminal_column, terminal_row);

    if (++terminal_column == terminal_text_width)
    {
        tty_breakline();
    }

    spinlock_unlock(&tty_lock);

    return c;
}

void tty_breakline()
{
    terminal_row++;
    terminal_column = 0;
    if (terminal_row == terminal_text_height)
    {
        tty_scroll();
    }
}

#define TAB_SIZE 4

void tty_tab()
{
    u8 spaces = TAB_SIZE - (terminal_column % TAB_SIZE);
    for (u8 i = 0; i < spaces; i++)
    {
        tty_putchar(' ');
    }
}

static inline bool is_ascii_character(char c)
{
    if (c >= ' ' && c <= '~') return true;

    return false;
}

void tty_write(const char *data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        tty_putchar(data[i]);
    }
    // spinlock_unlock(&tty_lock);
}

void tty_writestring(const char *data)
{
    tty_write(data, strlen(data));
}

void tty_backspace()
{
    if (is_cursor_after_input())
    {
        tty_putentryat(' ', tty_color, terminal_column - 1, terminal_row);
        terminal_column--;

        if (terminal_column == 0)
        {
            terminal_column = terminal_text_width;
            terminal_row--;
        }
    }
}

#define KEY_BACKSAPCE 0x0E
#define KEY_ENTER 0x1C

/* The line being edited. Tracking it directly is safe: scraping it back out of
 * text_buffer by cursor position blew up whenever scrolling drifted the
 * bookkeeping, overflowing the on-stack command buffer. */
#define TTY_LINE_MAX 256
static char tty_line[TTY_LINE_MAX];
static size_t tty_line_len;

void tty_handler_input(char scancode)
{
    if (scancode == KEY_BACKSAPCE)
    {
        if (tty_line_len > 0)
        {
            tty_line_len--;
            tty_backspace();
        }
        return;
    }

    if (scancode == KEY_ENTER)
    {
        tty_line[tty_line_len] = '\0';
        tty_line_len = 0;

        tty_breakline();
        runcmd(tty_line);
        kprint_prompt();
        return;
    }

    if ((unsigned char)scancode < 0x80) /* ignore break codes */
    {
        char c = convertScancode[(unsigned char)scancode];

        if (is_ascii_character(c) && tty_line_len < TTY_LINE_MAX - 1)
        {
            tty_line[tty_line_len++] = c;
            tty_putchar(c);

            if (terminal_column == terminal_text_width)
                tty_breakline();
        }
    }
}