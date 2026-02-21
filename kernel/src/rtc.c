#include <alias.h>
#include <helpers.h>
#include <io.h>
#include <ktime.h>
#include <rtc.h>
#include <stdio.h>
#include <types.h>

/* Ports for RTC communication*/
#define RTC_COMMAND_PORT 0x70
#define RTC_DATA_PORT 0x71

/* PIC Ports*/
#define PIC2_DATA 0xA1

static u8 bcd_to_binary(u8 bcd)
{
    return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

static u8 binary_to_bcd(u8 binary)
{
    return ((binary / 10) << 4) | (binary % 10);
}

/* static void rtc_write(u8 reg, u8 value)*/
/* {*/
/*	outb(RTC_COMMAND_PORT, reg);*/
/*	outb(RTC_DATA_PORT, value);*/
/* }*/

static inline int rtc_read_b(u8 reg)
{
    outb(RTC_COMMAND_PORT, reg);
    int result = inb(RTC_DATA_PORT);

    return bcd_to_binary(result);
}

static inline void rtc_write_b(u8 reg, u8 value)
{
    outb(RTC_COMMAND_PORT, reg);
    outb(RTC_DATA_PORT, binary_to_bcd(value));
}

void rtc_write(const RTCTimer *in)
{
    /* Disable updates while programming */
    outb(RTC_COMMAND_PORT, 0x8B); // select register B, disable NMI
    u8 prev = inb(RTC_DATA_PORT);
    outb(RTC_COMMAND_PORT, 0x8B);
    outb(RTC_DATA_PORT, prev | 0x80); // SET bit

    rtc_write_b(0x00, in->seconds);
    rtc_write_b(0x02, in->minutes);
    rtc_write_b(0x04, in->hours);
    rtc_write_b(0x07, in->day_of_month);
    rtc_write_b(0x08, in->month);
    rtc_write_b(0x09, in->year % 100);

    /* Re-enable updates */
    outb(RTC_COMMAND_PORT, 0x8B);
    outb(RTC_DATA_PORT, prev & ~0x80);
}

void rtc_read(RTCTimer *out)
{
    u8 *RTC_array = (u8 *)&out;

    for (u8 reg = 0; reg <= 0xD; reg++)
    {
        /* Write the register index to the RTC command port*/

        RTC_array[reg] = rtc_read_b(reg);
    }
}