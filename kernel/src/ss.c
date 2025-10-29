#include <ata.h>
#include <cpuid.h>
#include <helpers.h>
#include <pmm.h>
#include <rtc.h>
#include <stdio.h>
#include <string.h>
#include <tty.h>
#include <vga.h>
#include <framebuffer.h>

void kprint_prompt()
{
	tty_bg_color = VGA_COLOR_BLACK;
	tty_color = VGA_COLOR_GREEN;
	tty_writestring("root: ");
	tty_color = VGA_COLOR_WHITE;
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

static inline void lzfetch()
{

	u64 mem_ammount_mb = mem_ammount_b / (1024 * 1024);

	kprintf(" ____________________________\t\t\t\t\t\t\t\tLizard OS\n");
	kprintf("|					_		  |\t\t\t\t\t\t\t------------------\n");
	kprintf("|				   /\"\\		|\t\t\t\t\t\t\tKernel:	lz-kernel 0.1\n");
	kprintf("|				  /o o\\	   |\t\t\t\t\t\t\tUptime:  %ud %uH:%uM:%us\n", g_uptime_days,
			g_uptime_days, g_uptime_minutes, g_uptime_hours);
	kprintf("|			 _\\/  \\	/ \\/_	 |\t\t\t\t\t\t\tShell:	 shit-shell v0.0.3\n");
	kprintf("|			  \\\\._/  /_.//	|\t\t\t\t\t\t\tPackages: 5 (hardcoded)\n");
	kprintf("|			  `--,	,----'	  |\t\t\t\t\t\t\tResolution: %ux%u\n", width, height);
	kprintf("|				/	/		  |\t\t\t\t\t\t\tFont:	   bitmap_8x16\n");
	kprintf("|	  ^		   /	\\		   |\t\t\t\t\t\t\tTerminal: tty0\n");
	kprintf("|	 /|		  (		 )		  |\t\t\t\t\t\t\tTheme:	   CalangoGreen\n");
	kprintf("|	/ |		,__\\	 /__,	   |\t\t\t\t\t\t\tCPU:		%s\n", g_cpuid.brand_name);
	kprintf("|	\\ \\	_//---,	 ,--\\\\_	  |\t\t\t\t\t\t\tRAM:	   %u.%uMB\n", mem_ammount_mb);
	kprintf("|	 \\ \\	 /\\  /	 /	 /\\	  | \n");
	kprintf("|	  \\ \\.___,/  /			|\n");
	kprintf("|	   \\.______,/			   |\n");
	kprintf("|							  |\n");
	kprintf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	tty_color = VGA_COLOR_WHITE;
	tty_bg_color = VGA_COLOR_BLACK;
}

void free()
{
	#define BLOCK_SIZE 4096

	u32 free_blocks = pmm_free_block_count();
	u32 used_blocks = pmm_used_block_count();

	u32 free_kb = (free_blocks * BLOCK_SIZE) / 1024;
	u32 used_kb = (used_blocks * BLOCK_SIZE) / 1024;
	u32 total_kb = (usable_blocks * BLOCK_SIZE) / 1024;

	u32 free_mb = (free_kb + 512) / 1024;
	u32 used_mb = (used_kb + 512) / 1024;
	u32 total_mb = (total_kb + 512) / 1024;

	#define print_mem(value_kb, value_mb)															   \
		((value_mb > 10) ? (kprintf("%u MB", value_mb)) : (kprintf("%u KB", value_kb)))

	kprintf("Memory Available: ");
	print_mem(free_kb, free_mb);
	kprintf(" (%u blocos)\n", free_blocks);

	kprintf("Used Memory: ");
	print_mem(used_kb, used_mb);
	kprintf(" (%u blocos)\n", used_blocks);

	kprintf("Total Memory: ");
	print_mem(total_kb, total_mb);
	kprintf(" (%u blocos)\n", total_blocks);

	#undef print_mem
}

void uptime()
{
	kprintf("%ud %uH:%uM:%us\n", g_uptime_days, g_uptime_hours, g_uptime_minutes,
			g_uptime_seconds);
}

void ms()
{
	kprintf("[%u.%u]\n", g_uptime_seconds, g_uptime_milliseconds);
}

static inline void date()
{
	ClockTime time;
	
	clock_current(&time);
	clock_utc_to_local(&time);

	kprintf("%u/%u/%u %u:%u:%u\n", time.day, time.month,
			time.year, time.hour, time.minute, time.second);
}

/* Main */
#define CMD_IS(cmd, name) (strcmp(cmd, name) == 0)

void runcmd(const char *command)
{
	if (CMD_IS(command, "clear"))
	{
		clear();
	} else if (CMD_IS(command, "lzfetch"))
	{
		lzfetch();
	} else if (CMD_IS(command, "free"))
	{
		free();
	} else if (CMD_IS(command, "date"))
	{
		date();
	}
	else if (CMD_IS(command, "uptime"))
	{
		uptime();
	} 
	else if (CMD_IS(command, "ms"))
	{
		ms();
	}
	else
	{
		kprintf("Command not found: %s\n", command);
	}
}