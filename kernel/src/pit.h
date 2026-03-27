
#pragma once

#include <idt.h>
#include <task.h>

void isr_pit(CpuState *frame);

extern volatile u64 pit_ticks;