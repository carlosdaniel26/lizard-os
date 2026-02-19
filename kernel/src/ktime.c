#include <ktime.h>
#include <stddef.h>
#include <types.h>

static TimeSpec wall_clock; /* CLOCK_REALTIME */
static TimeSpec mono_clock; /* CLOCK_MONOTONIC */

static TimeSpec boot_wall;
static TimeSpec boot_mono;

static int is_leap_year(int y)
{
    return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
}

static const u16 days_before_month[2][12] = {
    /* normal */
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
    /* leap */
    {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}};

static u64 days_before_year(int y)
{
    u64 years = y - 1970;
    return years * 365 + (years + 1) / 4 - (years + 69) / 100 + (years + 369) / 400;
}

TimeSpec rtc_to_timespec(const RTCTimer *t)
{
    TimeSpec ts;

    u64 days = 0;

    days += days_before_year(t->year);
    days += days_before_month[is_leap_year(t->year)][t->month - 1];
    days += t->day_of_month - 1;

    ts.sec = days * 86400ULL + t->hours * 3600ULL + t->minutes * 60ULL + t->seconds;

    ts.nsec = 0;

    return ts;
}

void time_init_from_rtc(void)
{
    RTCTimer rtc;
    rtc_read(&rtc);

    wall_clock = rtc_to_timespec(&rtc); /* calendar -> seconds */
    mono_clock.sec = 0;
    mono_clock.nsec = 0;

    boot_wall = wall_clock;
    boot_mono = mono_clock;
}

void time_tick_ns(u64 delta_ns)
{
    /* monotonic */
    mono_clock.nsec += delta_ns;
    timespec_normalize(&mono_clock);

    /* wall clock follows monotonic */
    wall_clock.nsec += delta_ns;
    timespec_normalize(&wall_clock);
}

TimeSpec timespec_uptime(void)
{
    TimeSpec up;

    up.sec = mono_clock.sec - boot_mono.sec;
    up.nsec = mono_clock.nsec - boot_mono.nsec;

    timespec_normalize(&up);
    return up;
}

i64 time_uptime_ns(void)
{
    return timespec_to_ns(timespec_uptime(void));
}

i64 time_uptime_us(void)
{
    return timespec_to_us(timespec_uptime(void));
}

i64 time_uptime_ms(void)
{
    return timespec_to_ms(timespec_uptime(void));
}

TimeSpec time_now(void)
{
    return wall_clock;
}

static void timespec_split(const TimeSpec *ts, i32 *year, i32 *month, i32 *day, i32 *hour, i32 *min, i32 *sec)
{
    i64 s = ts->sec;

    i64 days = s / 86400;
    i64 rem = s % 86400;
    if (rem < 0)
    {
        rem += 86400;
        days--;
    }

    *hour = rem / 3600;
    rem %= 3600;
    *min = rem / 60;
    *sec = rem % 60;

    int y = 1970;
    while (1)
    {
        int diy = is_leap_year(y) ? 366 : 365;
        if (days < diy) break;
        days -= diy;
        y++;
    }

    static const int mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    int m = 0;
    while (1)
    {
        int dim = mdays[m];
        if (m == 1 && is_leap_year(y)) dim++;

        if (days < dim) break;

        days -= dim;
        m++;
    }

    if (year) *year = y;
    if (month) *month = m + 1;
    if (day) *day = days + 1;
}

i32 timespec_get_day(const TimeSpec *ts)
{
    i32 d;
    timespec_split(ts, NULL, NULL, &d, NULL, NULL, NULL);
    return d;
}

i32 timespec_get_month(const TimeSpec *ts)
{
    i32 m;
    timespec_split(ts, NULL, &m, NULL, NULL, NULL, NULL);
    return m;
}

i32 timespec_get_year(const TimeSpec *ts)
{
    i32 y;
    timespec_split(ts, &y, NULL, NULL, NULL, NULL, NULL);
    return y;
}

i32 timespec_get_hour(const TimeSpec *ts)
{
    i32 h;
    timespec_split(ts, NULL, NULL, NULL, &h, NULL, NULL);
    return h;
}

i32 timespec_get_min(const TimeSpec *ts)
{
    i32 m;
    timespec_split(ts, NULL, NULL, NULL, NULL, &m, NULL);
    return m;
}

i32 timespec_get_sec(const TimeSpec *ts)
{
    i32 s;
    timespec_split(ts, NULL, NULL, NULL, NULL, NULL, &s);
    return s;
}