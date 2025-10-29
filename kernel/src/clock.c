#include <clock.h>
#include <rtc.h>
#include <types.h>
#include <helpers.h>

#define UTC_OFFSET_BRAZIL -3

static ClockTime boot = {0};

int g_uptime_milliseconds = 0;
int g_uptime_seconds = 0;
int g_uptime_minutes = 0;
int g_uptime_hours = 0;
int g_uptime_days = 0;
int g_uptime_months = 0;
int g_uptime_years = 0;

static ClockTime clock = {0};
int utc_offset_hours = UTC_OFFSET_BRAZIL;

void save_boot_time()
{
	rtc_get_time(&boot);

    /* Also initialize clock to boot time */
    clock = boot;
}

void clock_current(ClockTime *out)
{
    *out = clock;
}

static void clock_refresh_uptime()
{
    g_uptime_years = clock.year - boot.year;
    g_uptime_months = clock.month - boot.month + g_uptime_years * 12;
    g_uptime_days = clock.day - boot.day + g_uptime_months * 30; /* Approximation */
    g_uptime_hours = clock.hour - boot.hour + g_uptime_days * 24;
    g_uptime_minutes = clock.minute - boot.minute + g_uptime_hours * 60;
    g_uptime_seconds = clock.second - boot.second + g_uptime_minutes * 60;
    g_uptime_milliseconds = clock.millisecond + g_uptime_seconds * 1000;
}

void clock_increase_ms()
{
    clock.millisecond++;
    if (clock.millisecond >= 1000)
    {
        clock.millisecond = 0;
        clock.second++;
        if (clock.second >= 60)
        {
            clock.second = 0;
            clock.minute++;
            if (clock.minute >= 60)
            {
                clock.minute = 0;
                clock.hour++;
                if (clock.hour >= 24)
                {
                    clock.hour = 0;
                    clock.day++;
                    int dim = days_in_month(clock.month, 2000 + clock.year);
                    if (clock.day > dim)
                    {
                        clock.day = 1;
                        clock.month++;
                        if (clock.month > 12)
                        {
                            clock.month = 1;
                            clock.year++;
                        }
                    }
                }
            }
        }
    }

    clock_refresh_uptime();
}

void clock_utc_to_local(ClockTime *time)
{
    time->hour += utc_offset_hours;
    if (time->hour >= 24)
    {
        time->hour -= 24;
        time->day++;
        int dim = days_in_month(time->month, 2000 + time->year);
        if (time->day > dim)
        {
            time->day = 1;
            time->month++;
            if (time->month > 12)
            {
                time->month = 1;
                time->year++;
            }
        }
    }
    else if (time->hour < 0)
    {
        time->hour += 24;
        time->day--;
        if (time->day < 1)
        {
            time->month--;
            if (time->month < 1)
            {
                time->month = 12;
                time->year--;
            }
            time->day = days_in_month(time->month, 2000 + time->year);
        }
    }
}