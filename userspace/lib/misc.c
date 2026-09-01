/* Odds and ends: the thin POSIX-ish shims doom's compiled sources reference
 * but that don't warrant their own file. Most are stubs - lizard has no
 * writable clock, directories, or TTY ioctls yet. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/syscall.h>

int usleep(unsigned usec)
{
    sys_sleep(usec / 1000);
    return 0;
}

int getpid(void)
{
    return sys_getpid();
}

int isatty(int fd)
{
    return fd == 0 || fd == 1 || fd == 2;
}

int mkdir(const char *path, int mode)
{
    (void)path;
    (void)mode;
    return -1; /* no directory creation on FAT16 yet */
}

int unlink(const char *path)
{
    (void)path;
    return -1;
}

int access(const char *path, int mode)
{
    (void)mode;
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    fclose(f);
    return 0;
}

time_t time(time_t *t)
{
    time_t v = (time_t)(sys_uptime_ms() / 1000);
    if (t)
        *t = v;
    return v;
}

struct tm *localtime(const time_t *t)
{
    static struct tm tm;
    long s = t ? *t : 0;
    tm.tm_sec = s % 60;
    tm.tm_min = (s / 60) % 60;
    tm.tm_hour = (s / 3600) % 24;
    tm.tm_mday = 1;
    tm.tm_mon = 0;
    tm.tm_year = 70;
    tm.tm_wday = 0;
    tm.tm_yday = 0;
    tm.tm_isdst = 0;
    return &tm;
}

size_t strftime(char *s, size_t max, const char *fmt, const struct tm *tm)
{
    (void)fmt;
    (void)tm;
    if (max)
        s[0] = '\0';
    return 0;
}

int gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    if (tv) {
        unsigned long ms = sys_uptime_ms();
        tv->tv_sec = ms / 1000;
        tv->tv_usec = (ms % 1000) * 1000;
    }
    return 0;
}

void __assert_fail(const char *expr, const char *file, int line)
{
    fprintf(stderr, "assert failed: %s (%s:%d)\n", expr, file, line);
    _exit(134);
}
