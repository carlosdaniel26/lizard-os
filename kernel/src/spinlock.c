#include <spinlock.h>

void spinlock_init(struct spinlock_t *lock)
{
    if (lock) lock->locked = 0;
}

void spinlock_lock(struct spinlock_t *lock)
{
    if (!lock) return;

    while (__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE))
    {
        while (lock->locked)
        {
            __asm__ volatile("pause" ::: "memory");
        }
    }
}

void spinlock_unlock(struct spinlock_t *lock)
{
    if (!lock) return;
    __atomic_clear(&lock->locked, __ATOMIC_RELEASE);
}

int spinlock_trylock(struct spinlock_t *lock)
{
    if (!lock) return 0;
    return !__atomic_test_and_set(&lock->locked, __ATOMIC_ACQUIRE);
}
