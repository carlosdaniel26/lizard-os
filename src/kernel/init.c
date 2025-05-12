#include <stdio.h>
#include <kernel/utils/alias.h>
#include <kernel/shit-shell/ss.h>

void init()
{
	debug_printf("PID1 on control\n");
	start_interrupts();
	shit_shell_init();

	while (1) {
		
	}
}
