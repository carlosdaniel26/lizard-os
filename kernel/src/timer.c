#include <timer.h>
#include <stddef.h>
#include <panic.h>

static struct timer_driver *active_timer = NULL;
static struct timer_driver *registered_timers = NULL;

void register_timer(struct timer_driver *driver)
{
    driver->next = registered_timers;
    registered_timers = driver;

    /* For now, auto-select the first registered driver as active */
    if (!active_timer) {
        active_timer = driver;
    }
}

int timer_init(void)
{
    if (active_timer && active_timer->init) {
        return active_timer->init();
    }
    return -1;
}

void timer_start(void)
{
    if (active_timer && active_timer->start) {
        active_timer->start();
    }
}

void timer_stop(void)
{
    if (active_timer && active_timer->stop) {
        active_timer->stop();
    }
}

u64 timer_read(void)
{
    if (active_timer && active_timer->read) {
        return active_timer->read();
    }
    return 0;
}
