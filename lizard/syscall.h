#pragma once

#include <abi/syscall.h>
#include <lizard/task.h>
#include <nolibc/types.h>

/* A syscall handler receives the six ABI argument registers and returns the
 * value that lands in the caller's RAX ( >= 0 ok, -errno on failure ). */
typedef long (*syscall_fn)(long a0, long a1, long a2, long a3, long a4, long a5);

/* Called from isr_syscall_stub (lizard/isr_vector.asm). */
void syscall_handler_c(struct cpu_state *regs);
