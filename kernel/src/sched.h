#pragma once

#include <task.h>
#include <types.h>

void isr_scheduler(CpuState *regs);
void scheduler();
void enable_scheduler();
void disable_scheduler();
u8 scheduler_is_enabled();