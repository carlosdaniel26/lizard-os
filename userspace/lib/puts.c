#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

int putchar(int c)
{
    char ch = (char)c;
    return sys_write(1, &ch, 1) == 1 ? c : -1;
}

int puts(const char *s)
{
    long n = sys_write(1, s, strlen(s));
    if (n < 0) return -1;
    return putchar('\n') < 0 ? -1 : (int)n + 1;
}
