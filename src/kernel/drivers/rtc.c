#include <stdint.h>
#include <stdio.h>

#include <kernel/drivers/rtc.h>
#include <kernel/utils/io.h>
#include <kernel/utils/alias.h>

// Ports for RTC communication
#define RTC_COMMAND_PORT 0x70
#define RTC_DATA_PORT 0x71

const char *months_strings[] = {
    "Undefined",
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

volatile uint32_t tick_count = 0;

struct RTC_timer RTC_clock;

void isr_timer()
{
    tick_count++;
    printf("Timer tick count: %u\n", tick_count);
}

void enable_rtc_interrupts()
{
    stop_interrupts();
    uint8_t rate = 0x00;
    // Enable RTC periodic interrupt
    outb(RTC_COMMAND_PORT, 0x8B);       // Select register B, disable NMI
    uint8_t prev = inb(RTC_DATA_PORT);  // Read current value of register B
    outb(RTC_COMMAND_PORT, 0x8B);       // Set the index again
    outb(RTC_DATA_PORT, prev | 0x40); // Set bit 6 of register B

    // Ensure rate is within valid range (2 to 15)
    rate &= 0x0F;

    // Set RTC frequency
    outb(RTC_COMMAND_PORT, 0x8A);       // Select register A, disable NMI
    prev = inb(RTC_DATA_PORT);       // Read current value of register A
    outb(RTC_COMMAND_PORT, 0x8A);       // Reset index to A
    outb(RTC_DATA_PORT, (prev & 0xF0) | rate); // Write only the rate to register A

    start_interrupts();
}

static uint8_t bcd_to_binary(uint8_t bcd)
{
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

static void rtc_write(uint8_t reg, uint8_t value)
{
	outb(RTC_COMMAND_PORT, reg);
	outb(RTC_DATA_PORT, value);
}

static uint8_t rtc_read(uint8_t reg)
{
	outb(RTC_COMMAND_PORT, reg);
	return inb(RTC_DATA_PORT);
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
    printf("Month: %s\n", months_strings[RTC_clock.month]);
    printf("Day: %u\n", RTC_clock.date_of_month);
    printf("Time: %u:%u:%u\n", RTC_clock.hours, RTC_clock.minutes, RTC_clock.seconds);
}
