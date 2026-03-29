#pragma once

#include <stdbool.h>
#include <types.h>

#define SPINLOCK_INITIALIZER                                                                                 \
    {                                                                                                        \
        0                                                                                                    \
    }

#define SPINLOCK(name) struct spinlock_t name = SPINLOCK_INITIALIZER

struct spinlock_t {
    volatile bool locked;
};

void spinlock_init(struct spinlock_t *lock);
void spinlock_lock(struct spinlock_t *lock);
void spinlock_unlock(struct spinlock_t *lock);
int spinlock_trylock(struct spinlock_t *lock);
