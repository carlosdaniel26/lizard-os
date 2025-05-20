#include <string.h>
#include <stdio.h>

#include <kernel/terminal/tty.h>
#include <kernel/terminal/vga.h>
#include <kernel/drivers/ata.h>
#include <kernel/drivers/rtc.h>
#include <kernel/cpu/cpuid.h>
#include <kernel/init.h>
#include <kernel/utils/helpers.h>

extern size_t cmd_start_column;
extern size_t cmd_start_row;
extern size_t terminal_row;
extern size_t terminal_column;

extern uint32_t height;
extern uint32_t width;

extern uint32_t terminal_color;
extern uint32_t terminal_background_color;
extern uint32_t mem_ammount_kb;

extern CPUID cpu;

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


static inline void lzfetch()
{

	struct Uptime time = {0};
	time = calculate_uptime();

    kprintf(" ____________________________|      Lizard OS\n");
    kprintf("|                  _         |      ------------------\n");
    kprintf("|                 /\"\\        |      Kernel:  lz-kernel 0.1\n");
    kprintf("|                /o o\\       |      Uptime:  %ud %uH:%uM:%us\n", time.days, time.hours, time.minutes, time.seconds);
    kprintf("|           _\\/  \\   / \\/_   |      Shell:   shit-shell v0.0.3\n");
    kprintf("|            \\\\._/  /_.//    |      Packages: 5 (hardcoded)\n");
    kprintf("|            `--,  ,----'    |      Resolution: %ux%u\n", width, height);
    kprintf("|              /   /         |      Font:     bitmap_8x16\n");
    kprintf("|    ^        /    \\         |      Terminal: tty0\n");
    kprintf("|   /|       (      )        |      Theme:    CalangoGreen\n");
    kprintf("|  / |     ,__\\    /__,      |      CPU:      %s\n", cpu.brand_name);
    kprintf("|  \\ \\   _//---,  ,--\\\\_     |      RAM:      %uMB\n", mem_ammount_kb / 1024);
    kprintf("|   \\ \\   /\\  /  /   /\\      | \n");
    kprintf("|    \\ \\.___,/  /            |\n");
    kprintf("|     \\.______,/             |\n");
    kprintf("|                            |\n");
    kprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    terminal_color = VGA_COLOR_WHITE;
    terminal_background_color = VGA_COLOR_BLACK;
}


/* Main */
#define CMD_IS(cmd, name) (strcmp(cmd, name) == 0)

void runcmd(const char *command)
{
	if (CMD_IS(command, "lsblk"))
	{
		lsblk();
	}
	else if (CMD_IS(command, "clear"))
	{
		clear();
	}
	else if (CMD_IS(command, "lzfetch"))
	{
		lzfetch();
	}
}
