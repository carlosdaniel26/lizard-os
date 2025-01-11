#include <stdint.h>
#include <stdio.h>

#include <kernel/drivers/rtc.h>
#include <kernel/utils/io.h>

#define NMI_COMMAND_PORT 0x70
#define NMI_DATA_PORT 0x71	

#define NMI_disable_bit 0x1

volatile uint32_t tick_count = 0;

struct RTC_timer RTC_clock;

void isr_timer()
{
	tick_count++;

	printf("OPA CHEGOU\n");
}

static uint8_t bcd_to_binary(uint8_t bcd)
{
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

void get_rtc_time()
{
	printf("\n");

	uint8_t *RTC_array = (uint8_t*) &RTC_clock;
	
	for (uint8_t i = 0; i <= 0xD; i++)
	{
		outb(NMI_COMMAND_PORT, (NMI_disable_bit << 7) | i);
		RTC_array[i] = bcd_to_binary(inb(NMI_DATA_PORT));

		printf("time 0x%x: %u\n", i, RTC_array[i]);
	}
}

void print_rtc_time()
{
	printf("year: 20%u\nmonth:%u\nday:%u\n", RTC_clock.year, RTC_clock.month, RTC_clock.date_of_month);
	printf("time: %u:%u:%u\n", RTC_clock.hours, RTC_clock.minutes, RTC_clock.seconds);
}