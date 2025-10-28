#include <clock.h>
#include <rtc.h>
#include <types.h>
#include <helpers.h>

static ClockTime boot = {0};

int g_uptime_milliseconds = 0;
int g_uptime_seconds = 0;
int g_uptime_minutes = 0;
int g_uptime_hours = 0;
int g_uptime_days = 0;
int g_uptime_months = 0;
int g_uptime_years = 0;

static ClockTime clock = {0};

void save_boot_time()
{
	boot = get_rtc_time();

	boot.year = RTC_clock.year;
    boot.month = RTC_clock.month;
    boot.day = RTC_clock.date_of_month;
    boot.hour = RTC_clock.hours;
    boot.minute = RTC_clock.minutes;
    boot.second = RTC_clock.seconds;
    
    /*
     * Milliseconds are not provided by RTC, PIT is responsible for that.
     * For now we set it to zero.
     */
    boot.millisecond = 0;

    /* Also initialize clock to boot time */
    clock = boot;
}

void clock_uptime(ClockTime *out)
{
    //calculate_uptime();
    *out = clock;
}

void clock_current(ClockTime *out)
{
    *out = get_rtc_time();
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