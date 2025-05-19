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
	ata_identify(1);

	char write_buf[512] = "HELLO ATA";
	ata_write_sector(1, 0, write_buf);

	char read_buf[512] = {0};
	ata_read_sector(1, 0, read_buf);

	debug_printf("Read from sector 0: %s", read_buf);

	shit_shell_init();

	infinite_loop();
}
