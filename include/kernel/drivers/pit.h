#ifndef PIT_H
#define PIT_H

#include <stdint.h>

void pit_init();
unsigned pit_read_count();
void isr_pit();

#endif