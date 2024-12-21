#define ptr_get_bit(A, B)    (((*(A)) >> (B & 7)) & 1)

#define ptr_set_bit(value, bit)   (*(value) |= (1UL << (bit)))
#define ptr_unset_bit(value, bit) (*(value) &= ~(1UL << (bit)))

void start_interrupts();
void stop_interrupts();