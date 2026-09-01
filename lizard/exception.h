#pragma once

#include <lizard/task.h>

/* Central handler for CPU vectors 0..31. Prints a full register dump + stack
 * backtrace, then either kills the offending user task or panics. Called from
 * isr_common_entry(). */
void exception_handle(struct cpu_state *regs);
