#include <alias.h>
#include <io.h>
#include <rtc.h>
#include <types.h>
#include <stdio.h>
#include <clock.h>

/* Ports for RTC communication*/
#define RTC_COMMAND_PORT 0x70
#define RTC_DATA_PORT 0x71

/* PIC Ports*/
#define PIC2_DATA 0xA1

/* Normalize */
#define NORMALIZE_RATE(rate)																	   \
	if ((rate) < 2 || (rate) > 15)																   \
	{																							   \
		(rate) = 6; /* Default frequency (1024 Hz) */											   \
	}

const char *months_strings[] = {"Undefined", "January",	 "February", "March",  "April",
								"May",		 "June",	 "July",	 "August", "September",
								"October",	 "November", "December"};

struct RTC_timer RTC_clock;

void isr_timer()
{
	outb(RTC_COMMAND_PORT, 0x0C);
	inb(RTC_DATA_PORT);
}

static ClockTime rtc_to_clock()
{
	ClockTime time;

	time.year = RTC_clock.year;
	time.month = RTC_clock.month;
	time.day = RTC_clock.date_of_month;
	time.hour = RTC_clock.hours;
	time.minute = RTC_clock.minutes;
	time.second = RTC_clock.seconds;
	time.millisecond = 0; /* RTC does not provide milliseconds */

	return time;
}

void enable_rtc_interrupts()
{
	u8 rate = 6;
	/* 1. Unmask IRQ8 in the PIC*/
	outb(PIC2_DATA, inb(PIC2_DATA) & ~0x01);

	/* 2. Enable RTC periodic interrupt*/
	outb(RTC_COMMAND_PORT, 0x8B);	   /* Select Register B*/
	u8 prev = inb(RTC_DATA_PORT); /* Read current value*/
	outb(RTC_COMMAND_PORT, 0x8B);	   /* Select again to write*/
	outb(RTC_DATA_PORT, prev | 0x40);  /* Set bit 6 (enable periodic interrupt)*/

	/* 3. Set the RTC update rate (frequency)*/
	NORMALIZE_RATE(rate);

	outb(RTC_COMMAND_PORT, 0x8A);			   /* Select Register A*/
	prev = inb(RTC_DATA_PORT);				   /* Read current value*/
	outb(RTC_COMMAND_PORT, 0x8A);			   /* Select again to write*/
	outb(RTC_DATA_PORT, (prev & 0xF0) | rate); /* Write only the rate part*/

	/* Clean RTC interrupts*/
	outb(RTC_COMMAND_PORT, 0x0C);
	inb(RTC_DATA_PORT); /* Read the data port to clear the interrupt*/
}

static u8 bcd_to_binary(u8 bcd)
{
	return ((bcd & 0xF0) >> 4) * 10 + (bcd & 0x0F);
}

/* static void rtc_write(u8 reg, u8 value)*/
/* {*/
/*	outb(RTC_COMMAND_PORT, reg);*/
/*	outb(RTC_DATA_PORT, value);*/
/* }*/

int rtc_read_b(u8 reg)
{
	outb(RTC_COMMAND_PORT, reg);
	int result = inb(RTC_DATA_PORT);

	return bcd_to_binary(result);
}

static void rtc_refresh_time()
{
	u8 *RTC_array = (u8 *)&RTC_clock;

	for (u8 reg = 0; reg <= 0xD; reg++)
	{
		/* Write the register index to the RTC command port*/

		RTC_array[reg] = rtc_read_b(reg);
	}
	
}

const char *get_month_string(int month_id)
{
	return months_strings[month_id];
}

ClockTime get_rtc_time()
{
	rtc_refresh_time();
	return rtc_to_clock();
}