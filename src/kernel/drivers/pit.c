#include <kernel/utils/io.h>
#include <kernel/utils/alias.h>

/* PIT operates in a 1.193.182 Hz frequency*/

#define PIT_COMMAND 0x43
#define PIT_CHANNEL0 0x40
#define PIT_FREQUENCY_HZ 1193182
#define DESIRED_FREQUENCY 1000 // 1ms timer
#define DESIRED_FREQUENCY_HZ (PIT_FREQUENCY_HZ / DESIRED_FREQUENCY)

void pit_init()
{
    outb(PIT_COMMAND, 0b00110110);      /* Mode 3, Channel 0, low/high byte acess*/
	
    outb(PIT_CHANNEL0, DESIRED_FREQUENCY_HZ & 0xFF);	/* Low Byte */
    outb(PIT_CHANNEL0, (DESIRED_FREQUENCY_HZ >> 8));	/* High Byte */
}