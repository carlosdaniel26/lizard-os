#pragma once

#include <types.h>

typedef struct ClockTime {
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
    int millisecond;
} ClockTime;

ClockTime get_uptime();
void save_boot_time();
void clock_uptime(ClockTime *out);
void clock_current(ClockTime *out);
void clock_increase_ms();

extern int g_uptime_milliseconds;
extern int g_uptime_seconds;
extern int g_uptime_minutes;
extern int g_uptime_hours;
extern int g_uptime_days;
extern int g_uptime_months;
extern int g_uptime_years;