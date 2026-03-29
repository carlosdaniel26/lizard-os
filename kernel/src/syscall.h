#pragma once

#include <task.h>
#include <types.h>

#define MAX_SYSCALLS 256

typedef void (*syscall_handler)(struct cpu_state *regs);

void isr_syscall(struct cpu_state *regs);

void sys_sleep(struct cpu_state *regs);
void syscall_handler_c(struct cpu_state *regs);

u64 syscall0(u64 syscall_num);
u64 syscall1(u64 syscall_num, u64 arg1);
u64 syscall2(u64 syscall_num, u64 arg1, u64 arg2);
