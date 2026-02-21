#include <helpers.h>
#include <rtc.h>
#include <types.h>

void hlt()
{
    while (1)
    {
        asm("sti");
        asm("hlt");
    }
}

void stop_cpu()
{
    while (1)
    {
        asm("cli");
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
    if (c >= 'a' && c <= 'z') return c - 32;

    return c;
}

int days_in_month(int month, int year)
{
    static const int days[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2)
    {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
    }
    return days[month - 1];
}