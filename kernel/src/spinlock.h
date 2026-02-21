#pragma once

#include <stdbool.h>
#include <types.h>

#define SPINLOCK_INITIALIZER {0}

#define SPINLOCK(name) spinlock_t name = SPINLOCK_INITIALIZER

typedef struct {
	volatile bool locked;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_lock(spinlock_t *lock);
void spinlock_unlock(spinlock_t *lock);
int spinlock_trylock(spinlock_t *lock);

