#include <string.h>
#include <stdio.h>

#include <kernel/terminal/tty.h>
#include <kernel/terminal/vga.h>
#include <kernel/drivers/ata.h>

extern size_t cmd_start_column;
extern size_t cmd_start_row;
extern size_t terminal_row;
extern size_t terminal_column;

extern uint32_t terminal_color;
extern uint32_t terminal_background_color;

void kprint_prompt()
{
	terminal_background_color = VGA_COLOR_BLACK;
	terminal_color = VGA_COLOR_GREEN;
	tty_writestring("root: ");
	terminal_color = VGA_COLOR_WHITE;
	cmd_start_column = terminal_column;
	cmd_start_row = terminal_row;
}

void shit_shell_init()
{
	kprint_prompt();
}

/* Commands */

static inline void clear()
{
	tty_clean();
}

static inline void lsblk()
{
	for(uint8_t i = 0; i <= 2; i++)
	{
		ATADevice *dev = ata_get(i);
		if (dev->present == 0) continue;

		kprintf("HDD %u\n", i + 1);
		kprintf("%s\n", dev->model);
		kprintf("MB: %u\n", (dev->total_bytes  / (1024 * 1024)) + 1);
	}
}

/* Main */

void shell(const char *command)
{
	if(memcmp(command, "lsblk", strlen("lsblk")) == 1)
	{
		lsblk();
	}
	else if (memcmp(command, "clear", strlen("clear")) == 1)
	{
		clear();
	}
}