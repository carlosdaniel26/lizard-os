#include <alias.h>
#include <io.h>
#include <rtc.h>
#include <types.h>
#include <stdio.h>

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

u8 rtc_read_b(u8 reg)
{
	outb(RTC_COMMAND_PORT, reg);
	u8 result = inb(RTC_DATA_PORT);

	return bcd_to_binary(result);
}

void utc_to_local()
{
	/* Convert to UTC - 3 */
	if (RTC_clock.hours >= 3)
	{
		RTC_clock.hours -= 3;
	}
	else
	{
		RTC_clock.hours = (RTC_clock.hours + 24) - 3;
		/* Previous day */
		if (RTC_clock.date_of_month > 1)
		{
			RTC_clock.date_of_month--;
		}
		else
		{
			/* Move to previous month */
			if (RTC_clock.month == 1)
			{
				RTC_clock.month = 12;
				RTC_clock.year--;
			}
			else
			{
				RTC_clock.month--;
			}

			/* Set the correct date_of_month */
			switch (RTC_clock.month)
			{
			case 1: /* January */
			case 3: /* March */
			case 5: /* May */
			case 7: /* July */
			case 8: /* August */
			case 10: /* October */
			case 12: /* December */
				RTC_clock.date_of_month = 31;
				break;
			case 4: /* April */
			case 6: /* June */
			case 9: /* September */
			case 11: /* November */
				RTC_clock.date_of_month = 30;
				break;
			case 2: /* February */
				/* Check for leap year*/
				if ((RTC_clock.year % 4 == 0 && RTC_clock.year % 100 != 0) ||
					(RTC_clock.year % 400 == 0))
				{
					RTC_clock.date_of_month = 29;
				}
				else
				{
					RTC_clock.date_of_month = 28;
				}
				break;
			default:
				RTC_clock.date_of_month = 31; /* Fallback, should not happen*/
				break;
			}
		}
	}
}

void rtc_refresh_time()
{
	u8 *RTC_array = (u8 *)&RTC_clock;

	for (u8 reg = 0; reg <= 0xD; reg++)
	{
		/* Write the register index to the RTC command port*/

		RTC_array[reg] = rtc_read_b(reg);
	}
	
	utc_to_local();
}

const char *get_month_string(u8 month_id)
{
	return months_strings[month_id];
}