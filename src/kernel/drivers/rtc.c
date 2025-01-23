#include <stdint.h>
#include <stdio.h>

#include <kernel/drivers/rtc.h>
#include <kernel/utils/io.h>

// Ports for RTC communication
#define RTC_COMMAND_PORT 0x70
#define RTC_DATA_PORT 0x71

volatile uint32_t tick_count = 0;

struct RTC_timer RTC_clock;

void isr_timer()
{
    tick_count++;
    printf("Timer tick count: %u\n", tick_count);
}

static uint8_t bcd_to_binary(uint8_t bcd)
{
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

void get_rtc_time()
{
    printf("\nReading RTC Time:\n");

    uint8_t *RTC_array = (uint8_t *) &RTC_clock;

    for (uint8_t reg = 0; reg <= 0xD; reg++)
    {
        // Write the register index to the RTC command port
        outb(RTC_COMMAND_PORT, reg);
        
        // Read the data from the RTC data port
        RTC_array[reg] = bcd_to_binary(inb(RTC_DATA_PORT));

        printf("Register 0x%X: %u\n", reg, RTC_array[reg]);
    }
}

void print_rtc_time()
{
    printf("Current RTC Time:\n");
    printf("Year: 20%u\n", RTC_clock.year);
    printf("Month: %u\n", RTC_clock.month);
    printf("Day: %u\n", RTC_clock.date_of_month);
    printf("Time: %02u:%02u:%02u\n", RTC_clock.hours, RTC_clock.minutes, RTC_clock.seconds);
}
