#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
	volatile bool locked;
} spinlock_t;

void spinlock_init(spinlock_t *lock);
void spinlock_acquire(spinlock_t *lock);
void spinlock_release(spinlock_t *lock);

