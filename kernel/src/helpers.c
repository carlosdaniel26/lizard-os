#include <helpers.h>
#include <rtc.h>
#include <stdint.h>

void hlt()
{
	while (1)
	{
		asm("sti");
		asm("hlt");
	}
}

int oct2bin(unsigned char *str, int size)
{
	int n = 0;
	unsigned char *c = str;
	while (size-- > 0)
	{
		n *= 8;
		n += *c - '0';
		c++;
	}
	return n;
}

char toupper(char c)
{
	if (c >= 'a' && c <= 'z')
		return c - 32;

	return c;
}

struct RTC_timer boot_time;

static uint8_t days_in_month(uint8_t month, uint8_t year)
{
	static const uint8_t days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (month == 2)
	{
		if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
			return 29;
	}
	return days[month - 1];
}

void save_boot_time()
{
	rtc_refresh_time();
	boot_time = RTC_clock;
}

struct Uptime calculate_uptime()
{
	struct Uptime up = {0};

	rtc_refresh_time();

	int sec = RTC_clock.seconds - boot_time.seconds;
	int min = RTC_clock.minutes - boot_time.minutes;
	int hour = RTC_clock.hours - boot_time.hours;
	int day = RTC_clock.date_of_month - boot_time.date_of_month;
	int month = RTC_clock.month - boot_time.month;
	int year = RTC_clock.year - boot_time.year;

	if (sec < 0)
	{
		sec += 60;
		min--;
	}
	if (min < 0)
	{
		min += 60;
		hour--;
	}
	if (hour < 0)
	{
		hour += 24;
		day--;
	}
	if (day < 0)
	{
		month--;
		if (month < 0)
		{
			month += 12;
			year--;
		}
		day +=
			days_in_month((boot_time.month == 1 ? 12 : boot_time.month - 1), 2000 + boot_time.year);
	}
	if (month < 0)
	{
		month += 12;
		year--;
	}

	up.seconds = sec;
	up.minutes = min;
	up.hours = hour;
	up.days = day;
	up.months = month;
	up.years = year;

	return up;
}