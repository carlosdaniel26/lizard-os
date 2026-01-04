#pragma once

#include <types.h>
#include <stdio.h>
#include <ktime.h>
#include <pit.h>

#define debug_printf(fmt, ...)                                                 \
    do {                                                                       \
        TimeSpec ts = timespec_uptime();                                       \
        u64 ms = (u64)(ts.nsec / 1000000);                                      \
        kprintf("[%llu.", (u64)ts.sec);                                         \
        if (ms < 10)                                                           \
            kprintf("00");                                                     \
        else if (ms < 100)                                                     \
            kprintf("0");                                                      \
        kprintf("%llu] %s:%u: " fmt, ms, __FILE__, __LINE__, ##__VA_ARGS__);     \
    } while (0)
