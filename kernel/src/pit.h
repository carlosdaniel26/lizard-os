#ifndef PIT_H
#define PIT_H

#include <idt.h>
#include <task.h>

void pit_init();
void isr_pit(CpuState *frame);

#endif