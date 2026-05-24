#pragma once

#include <types.h>

struct timer_driver {
    const char *name;
    int (*init)(void);
    void (*start)(void);
    void (*stop)(void);
    u64 (*read)(void);
    struct timer_driver *next;
};

void register_timer(struct timer_driver *driver);
int timer_init(void);
void timer_start(void);
void timer_stop(void);
u64 timer_read(void);
