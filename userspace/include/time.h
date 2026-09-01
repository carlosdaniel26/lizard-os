#pragma once
#include <sys/types.h>

typedef long time_t;

struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};

time_t time(time_t *t);
struct tm *localtime(const time_t *t);
size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm);
