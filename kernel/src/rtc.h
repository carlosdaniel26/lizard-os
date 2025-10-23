#ifndef RTC_H
#define RTC_H

struct RTC_timer {
	uint8_t seconds;	   /* 00*/
	uint8_t seconds_alarm; /* 01*/
	uint8_t minutes;	   /* 02*/
	uint8_t minutes_alarm; /* 03*/
	uint8_t hours;		   /* 04*/
	uint8_t hours_alarm;   /* 05*/
	uint8_t day_in_week;   /* 06*/
	uint8_t date_of_month; /* 07*/
	uint8_t month;		   /* 08*/
	uint8_t year;		   /* 09*/

	uint8_t status_register_A; /* A*/
	uint8_t status_register_B; /* B*/
	uint8_t status_register_C; /* C*/
	uint8_t status_register_D; /* D*/
};

void isr_timer();
void enable_rtc_interrupts();
void rtc_refresh_time();
void kprint_rtc_time();
const char *get_month_string(uint8_t month_id);

#endif