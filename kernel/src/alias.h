#ifndef ALIAS_H
#define ALIAS_H

#define NULL_POINTER 0x00

#define ptr_get_bit(A, B) (((*(A)) >> (B & 7)) & 1)

#define ptr_set_bit(value, bit) (*(value) |= (1UL << (bit)))
#define ptr_unset_bit(value, bit) (*(value) &= ~(1UL << (bit)))

#define start_interrupts() asm volatile("sti")
#define stop_interrupts() asm volatile("cli")

#define die() asm("hlt")
#define infinite_loop()                                                                            \
    while (1) {                                                                                    \
    }

#endif