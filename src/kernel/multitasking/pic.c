#include <stdbool.h>
#include <stdint.h>

#include <kernel/multitasking/pic.h>

#define NULL_PID 0x00
#define UNDEFINED_PID 0x00

#define MIN_PID 0x01
#define MAX_PID 0XFFFF /* Max 16 bits Address */

bool pids[MAX_PID];

int16_t alloc_pid()
{
	for (uint32_t i = 1; i < MAX_PID; i++)
	{
		if (pids[i] == false)
		{
			pids[i] = true;
			return i;
		}
	}

	return 0;
}

uint8_t free_pid(uint32_t pid)
{
	pids[pid] = false;

	return 0;
}