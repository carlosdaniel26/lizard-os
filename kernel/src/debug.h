#pragma once

#include <types.h>
#include <stdio.h>
#include <clock.h>

#define debug_printf(fmt, ...) kprintf("[%u.%u] %s:%u: " fmt, g_uptime_seconds,pit_milliseconds,__FILE__, __LINE__, ##__VA_ARGS__)