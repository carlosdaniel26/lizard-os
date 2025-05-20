#include <stdio.h>
#include <stdint.h>
#include <kernel/utils/alias.h>
#include <kernel/utils/helpers.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/terminal/tty.h>
#include <kernel/terminal/vga.h>

#include <kernel/drivers/ata.h>
#include <kernel/drivers/rtc.h>

#include <kernel/cpu/cpuid.h>

void init()
{
	save_boot_time();
	tty_initialize();

	start_interrupts();
	ata_detect_devices();
	
	cpuid_get_brand();

	shit_shell_init();

	infinite_loop();
}
