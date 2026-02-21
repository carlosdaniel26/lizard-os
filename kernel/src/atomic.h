#pragma once

#include <stdbool.h>
#include <types.h>

typedef struct {
    volatile i64 count;
} atomic_t;

#define ATOMIC_INIT(i)                                                                                       \
    {                                                                                                        \
        (i)                                                                                                  \
    }
#define atomic_init(v, i) atomic_set(v, i)

static inline i64 atomic_read(const atomic_t *v)
{
    return __atomic_load_n(&v->count, __ATOMIC_RELAXED);
}

static inline void atomic_set(atomic_t *v, i64 i)
{
    __atomic_store_n(&v->count, i, __ATOMIC_RELAXED);
}

static inline void atomic_add(i64 i, atomic_t *v)
{
    __atomic_add_fetch(&v->count, i, __ATOMIC_SEQ_CST);
}

static inline void atomic_sub(i64 i, atomic_t *v)
{
    __atomic_sub_fetch(&v->count, i, __ATOMIC_SEQ_CST);
}

static inline void atomic_inc(atomic_t *v)
{
    atomic_add(1, v);
}

static inline void atomic_dec(atomic_t *v)
{
    atomic_sub(1, v);
}

static inline i64 atomic_add_return(i64 i, atomic_t *v)
{
    return __atomic_add_fetch(&v->count, i, __ATOMIC_SEQ_CST);
}

static inline i64 atomic_sub_return(i64 i, atomic_t *v)
{
    return __atomic_sub_fetch(&v->count, i, __ATOMIC_SEQ_CST);
}

static inline i64 atomic_inc_return(atomic_t *v)
{
    return atomic_add_return(1, v);
}

static inline i64 atomic_dec_return(atomic_t *v)
{
    return atomic_sub_return(1, v);
}

static inline i64 atomic_fetch_add(i64 i, atomic_t *v)
{
    return __atomic_fetch_add(&v->count, i, __ATOMIC_SEQ_CST);
}

static inline i64 atomic_fetch_sub(i64 i, atomic_t *v)
{
    return __atomic_fetch_sub(&v->count, i, __ATOMIC_SEQ_CST);
}

static inline i64 atomic_cmpxchg(atomic_t *v, i64 old, i64 new)
{
    __atomic_compare_exchange_n(&v->count, &old, new, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return old;
}

static inline i64 atomic_xchg(atomic_t *v, i64 new)
{
    return __atomic_exchange_n(&v->count, new, __ATOMIC_SEQ_CST);
}

static inline bool atomic_add_unless(atomic_t *v, i64 a, i64 u)
{
    i64 c = atomic_read(v);
    while (c != u &&
           !__atomic_compare_exchange_n(&v->count, &c, c + a, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
    {
    }
    return c != u;
}

static inline bool atomic_inc_not_zero(atomic_t *v)
{
    return atomic_add_unless(v, 1, 0);
}

static inline bool atomic_sub_and_test(i64 i, atomic_t *v)
{
    return atomic_sub_return(i, v) == 0;
}

static inline bool atomic_dec_and_test(atomic_t *v)
{
    return atomic_dec_return(v) == 0;
}

static inline bool atomic_inc_and_test(atomic_t *v)
{
    return atomic_inc_return(v) == 0;
}

static inline bool atomic_add_negative(i64 i, atomic_t *v)
{
    return atomic_add_return(i, v) < 0;
}
