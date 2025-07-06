#ifndef PIT_H
#define PIT_H

#include <idt.h>

void init_pit();
void isr_pit(InterruptFrame *frame);

#endif