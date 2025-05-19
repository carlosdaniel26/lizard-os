#include <stdio.h>
#include <stdint.h>
#include <kernel/utils/alias.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/terminal/tty.h>

#include <kernel/drivers/ata.h>

void init()
{
	tty_initialize();
	debug_printf("PID1 on control");

	start_interrupts();
	ata_detect_devices();
	
	shit_shell_init();

	infinite_loop();
}
