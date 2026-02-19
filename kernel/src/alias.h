#pragma once

#define NULL_POINTER 0x00

#define ptr_get_bit(A, B) (((*(A)) >> (B & 7)) & 1)

#define ptr_set_bit(value, bit) (*(value) |= (1UL << (bit)))
#define ptr_unset_bit(value, bit) (*(value) &= ~(1UL << (bit)))

#define start_interrupts(void) asm volatile("sti")
#define stop_interrupts(void) asm volatile("cli")

#define die(void) asm("hlt")
#define infinite_loop(void)                                                                                  \
    while (1)                                                                                                \
    {                                                                                                        \
    }
