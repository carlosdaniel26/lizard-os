#include <stdio.h>
#include <stdint.h>
#include <kernel/utils/alias.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/terminal/tty.h>

void init()
{
	tty_initialize();
	debug_printf("PID1 on control\n");
	ata_identify(1);

	start_interrupts();
	shit_shell_init();

	infinite_loop();
}
