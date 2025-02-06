#include <stdint.h>
#include <stdio.h>

#include <kernel/drivers/rtc.h>
#include <kernel/utils/io.h>
#include <kernel/utils/alias.h>

// Ports for RTC communication
#define RTC_COMMAND_PORT 0x70
#define RTC_DATA_PORT 0x71

/* PIC Ports*/
#define PIC2_DATA 0xA1

/* Normalize */
#define NORMALIZE_RATE(rate) \
        if ((rate) < 2 || (rate) > 15) { \
            (rate) = 6; /* Default frequency (1024 Hz) */ \
        } \


const char *months_strings[] = {
    "Undefined",
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

struct RTC_timer RTC_clock;

void isr_timer()
{
    outb(RTC_COMMAND_PORT, 0x0C);
    inb(RTC_DATA_PORT);
}

void enable_rtc_interrupts() 
{
    uint8_t rate = 6;
    // 1. Unmask IRQ8 in the PIC
    outb(PIC2_DATA, inb(PIC2_DATA) & ~ 0x01);

    // 2. Enable RTC periodic interrupt
    outb(RTC_COMMAND_PORT, 0x8B);       // Select Register B
    uint8_t prev = inb(RTC_DATA_PORT);  // Read current value
    outb(RTC_COMMAND_PORT, 0x8B);       // Select again to write
    outb(RTC_DATA_PORT, prev | 0x40);   // Set bit 6 (enable periodic interrupt)

    // 3. Set the RTC update rate (frequency)
   NORMALIZE_RATE(rate);

    outb(RTC_COMMAND_PORT, 0x8A);       // Select Register A
    prev = inb(RTC_DATA_PORT);          // Read current value
    outb(RTC_COMMAND_PORT, 0x8A);       // Select again to write
    outb(RTC_DATA_PORT, (prev & 0xF0) | rate); // Write only the rate part

    /* Clean RTC interrupts*/
    outb(RTC_COMMAND_PORT, 0x0C);
    inb(RTC_DATA_PORT); // Read the data port to clear the interrupt
}

static uint8_t bcd_to_binary(uint8_t bcd)
{
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

// static void rtc_write(uint8_t reg, uint8_t value)
// {
// 	outb(RTC_COMMAND_PORT, reg);
// 	outb(RTC_DATA_PORT, value);
// }

static uint8_t rtc_read_b(uint8_t reg)
{
	outb(RTC_COMMAND_PORT, reg);
	uint8_t result = inb(RTC_DATA_PORT);

    return bcd_to_binary(result);
}

void get_rtc_time()
{
    printf("\nReading RTC Time:\n");

    uint8_t *RTC_array = (uint8_t *) &RTC_clock;

    for (uint8_t reg = 0; reg <= 0xD; reg++)
    {
        // Write the register index to the RTC command port
        
        RTC_array[reg] = rtc_read_b(reg);

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
