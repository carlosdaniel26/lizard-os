
#pragma once

#include <idt.h>
#include <task.h>

void pit_stop();
void pit_start();
void isr_pit(struct cpu_state *frame);

extern volatile u64 pit_ticks;