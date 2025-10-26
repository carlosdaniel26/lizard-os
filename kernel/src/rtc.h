#pragma once

struct RTC_timer {
	u8 seconds;	   /* 00*/
	u8 seconds_alarm; /* 01*/
	u8 minutes;	   /* 02*/
	u8 minutes_alarm; /* 03*/
	u8 hours;		   /* 04*/
	u8 hours_alarm;   /* 05*/
	u8 day_in_week;   /* 06*/
	u8 date_of_month; /* 07*/
	u8 month;		   /* 08*/
	u8 year;		   /* 09*/

	u8 status_register_A; /* A*/
	u8 status_register_B; /* B*/
	u8 status_register_C; /* C*/
	u8 status_register_D; /* D*/
};

extern struct RTC_timer RTC_clock;

void isr_timer();
void enable_rtc_interrupts();
void rtc_refresh_time();
void kprint_rtc_time();
const char *get_month_string(u8 month_id);

