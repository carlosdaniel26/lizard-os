#include <string.h>
#include <stdio.h>

#include <tty.h>
#include <vga.h>

extern size_t cmd_start_column;
extern size_t cmd_start_row;
extern size_t terminal_row;
extern size_t terminal_column;

extern uint32_t height;
extern uint32_t width;

extern uint32_t terminal_color;
extern uint32_t terminal_background_color;
extern uint64_t mem_ammount_kb;
extern uint32_t total_blocks;

//extern CPUID cpu;

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
	// for(uint8_t i = 0; i <= 2; i++)
	// {
	// 	ATADevice *dev = ata_get(i);
	// 	if (dev->present == 0) continue;

	// 	kprintf("HDD %u\n", i + 1);
	// 	kprintf("%s\n", dev->model);
	// 	kprintf("MB: %u\n", (dev->total_bytes  / (1024 * 1024)) + 1);
	// }
}


static inline void lzfetch()
{

	// struct Uptime time = {0};
	// time = calculate_uptime();

    // kprintf(" ____________________________\t\t\t\t\t\t\t\tLizard OS\n");
    // kprintf("|                  _         |\t\t\t\t\t\t\t------------------\n");
    // kprintf("|                 /\"\\        |\t\t\t\t\t\t\tKernel:  lz-kernel 0.1\n");
    // kprintf("|                /o o\\       |\t\t\t\t\t\t\tUptime:  %ud %uH:%uM:%us\n", time.days, time.hours, time.minutes, time.seconds);
    // kprintf("|           _\\/  \\   / \\/_   |\t\t\t\t\t\t\tShell:   shit-shell v0.0.3\n");
    // kprintf("|            \\\\._/  /_.//    |\t\t\t\t\t\t\tPackages: 5 (hardcoded)\n");
    // kprintf("|            `--,  ,----'    |\t\t\t\t\t\t\tResolution: %ux%u\n", width, height);
    // kprintf("|              /   /         |\t\t\t\t\t\t\tFont:     bitmap_8x16\n");
    // kprintf("|    ^        /    \\         |\t\t\t\t\t\t\tTerminal: tty0\n");
    // kprintf("|   /|       (      )        |\t\t\t\t\t\t\tTheme:    CalangoGreen\n");
    // kprintf("|  / |     ,__\\    /__,      |\t\t\t\t\t\t\tCPU:      %s\n", cpu.brand_name);
    // kprintf("|  \\ \\   _//---,  ,--\\\\_     |\t\t\t\t\t\t\tRAM:      %uMB\n", mem_ammount_kb / 1024);
    // kprintf("|   \\ \\   /\\  /  /   /\\      | \n");
    // kprintf("|    \\ \\.___,/  /            |\n");
    // kprintf("|     \\.______,/             |\n");
    // kprintf("|                            |\n");
    // kprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    // terminal_color = VGA_COLOR_WHITE;
    // terminal_background_color = VGA_COLOR_BLACK;
}

void free()
{
	// #define BLOCK_SIZE_KB 4096

    // uint32_t free_blocks = pmm_free_block_count();
    // uint32_t used_blocks = total_blocks - free_blocks;

    // uint32_t free_kb = free_blocks * BLOCK_SIZE_KB;
    // uint32_t used_kb = used_blocks * BLOCK_SIZE_KB;
    // uint32_t total_kb = total_blocks * BLOCK_SIZE_KB;

    // uint32_t free_mb = (free_kb + 512) / 1024;
    // uint32_t used_mb = (used_kb + 512) / 1024;
    // uint32_t total_mb = (total_kb + 512) / 1024;

    // #define print_mem(value_kb, value_mb) ((value_mb > 0) ? (kprintf("%u MB", value_mb)) : (kprintf("%u KB", value_kb)))

    // kprintf("Memory Available: ");
    // print_mem(free_kb, free_mb);
    // kprintf(" (%u blocos)\n", free_blocks);

    // kprintf("Used Memory: ");
    // print_mem(used_kb, used_mb);
    // kprintf(" (%u blocos)\n", used_blocks);

    // kprintf("Total Memory: ");
    // print_mem(total_kb, total_mb);
    // kprintf(" (%u blocos)\n", total_blocks);


    // #undef print_mem
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
	else if (CMD_IS(command, "free"))
	{
		free();
	}
}