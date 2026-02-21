#pragma once
#include <types.h>
#include <rtc.h>

#define NSEC_PER_SEC   1000000000LL
#define USEC_PER_SEC   1000000LL
#define MSEC_PER_SEC   1000LL

#define NSEC_PER_MSEC  1000000LL
#define NSEC_PER_USEC  1000LL
#define USEC_PER_MSEC  1000LL

typedef i64 time_t;     /* epoch */

typedef struct TimeSpec {
    i64  sec;
    i64  nsec;  /* [0, NSEC_PER_SEC) */
} TimeSpec;

#define timespec_is_normalized(ts) \
    ((ts)->nsec >= 0 && (ts)->nsec < NSEC_PER_SEC)

#define timespec_is_zero(ts) \
    ((ts)->sec == 0 && (ts)->nsec == 0)


static inline void timespec_normalize(TimeSpec *ts)
{
    if (ts->nsec >= NSEC_PER_SEC || ts->nsec <= -NSEC_PER_SEC) {
        ts->sec  += ts->nsec / NSEC_PER_SEC;
        ts->nsec %= NSEC_PER_SEC;
    }

    if (ts->nsec < 0) {
        ts->nsec += NSEC_PER_SEC;
        ts->sec--;
    }
}

static inline i64 timespec_to_ns(TimeSpec ts)
{
    return ts.sec * NSEC_PER_SEC + ts.nsec;
}

static inline i64 timespec_to_us(TimeSpec ts)
{
    return ts.sec * USEC_PER_SEC + ts.nsec / NSEC_PER_USEC;
}

static inline i64 timespec_to_ms(TimeSpec ts)
{
    return ts.sec * MSEC_PER_SEC + ts.nsec / NSEC_PER_MSEC;
}

static inline TimeSpec timespec_from_ns(i64 ns)
{
    TimeSpec ts;

    ts.sec  = ns / NSEC_PER_SEC;
    ts.nsec = ns % NSEC_PER_SEC;

    if (ts.nsec < 0) {
        ts.nsec += NSEC_PER_SEC;
        ts.sec--;
    }

    return ts;
}

static inline TimeSpec timespec_from_us(i64 us)
{
    TimeSpec ts;

    ts.sec  = us / USEC_PER_SEC;
    ts.nsec = (us % USEC_PER_SEC) * NSEC_PER_USEC;

    return ts;
}

static inline TimeSpec timespec_from_ms(i64 ms)
{
    TimeSpec ts;

    ts.sec  = ms / MSEC_PER_SEC;
    ts.nsec = (ms % MSEC_PER_SEC) * NSEC_PER_MSEC;

    return ts;
}

static inline TimeSpec timespec_add(TimeSpec a, TimeSpec b)
{
    TimeSpec r = {
        .sec  = a.sec + b.sec,
        .nsec = a.nsec + b.nsec
    };
    timespec_normalize(&r);
    return r;
}

static inline TimeSpec timespec_sub(TimeSpec a, TimeSpec b)
{
    TimeSpec r = {
        .sec  = a.sec - b.sec,
        .nsec = a.nsec - b.nsec
    };
    timespec_normalize(&r);
    return r;
}

#define timespec_eq(a, b) \
    ((a).sec == (b).sec && (a).nsec == (b).nsec)

#define timespec_lt(a, b) \
    ((a).sec < (b).sec || \
    ((a).sec == (b).sec && (a).nsec < (b).nsec))

#define timespec_gt(a, b) timespec_lt(b, a)
#define timespec_le(a, b) (!timespec_gt(a, b))
#define timespec_ge(a, b) (!timespec_lt(a, b))

TimeSpec rtc_to_timespec(const RTCTimer *t);

void time_init_from_rtc(void);
void time_tick_ns(u64 delta_ns);
TimeSpec timespec_uptime(void);
i64 time_uptime_ns(void);
i64 time_uptime_us(void);
i64 time_uptime_ms(void);
TimeSpec time_now(void);

/* Calendar projections */
i32 timespec_get_year(const TimeSpec *ts);
i32 timespec_get_month(const TimeSpec *ts);
i32 timespec_get_day(const TimeSpec *ts);
i32 timespec_get_hour(const TimeSpec *ts);
i32 timespec_get_min(const TimeSpec *ts);
i32 timespec_get_sec(const TimeSpec *ts);

extern const char *months_strings[];
