#pragma once

#include <lizard/ktime.h>
#include <lizard/pit.h>
#include <nolibc/stdio.h>
#include <nolibc/types.h>

#define debug_printf(fmt, ...)                                                                               \
    do                                                                                                       \
    {                                                                                                        \
        struct time_spec ts = timespec_uptime();                                                                     \
        u64 ms = (u64)(ts.nsec / 1000000);                                                                   \
        kprintf("[%llu.", (u64)ts.sec);                                                                      \
        if (ms < 10)                                                                                         \
            kprintf("00");                                                                                   \
        else if (ms < 100)                                                                                   \
            kprintf("0");                                                                                    \
        kprintf("%llu] %s:%u: " fmt, ms, __FILE__, __LINE__, ##__VA_ARGS__);                                 \
    }                                                                                                        \
    while (0)
