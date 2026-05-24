#pragma once

#include <lizard/task.h>
#include <nolibc/types.h>

void isr_scheduler(struct cpu_state *regs);
void scheduler();
void enable_scheduler();
void disable_scheduler();
u8 scheduler_is_enabled();