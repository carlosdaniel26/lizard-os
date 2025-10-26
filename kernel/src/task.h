#pragma once

#include <types.h>

#define TASK_NAME_MAX_LEN 32

typedef struct __attribute__((packed)) CpuState {
	u64 rax;
	u64 rdi;
	u64 rsi;
	u64 rdx;
	u64 rcx;
	u64 r8;
	u64 r9;
	u64 r10;
	u64 r11;
	u64 rbx;
	u64 rbp;
	u64 r12;
	u64 r13;
	u64 r14;
	u64 r15;

	/* Interrupt Frame */
	u64 rip;
	u64 cs;
	u64 rflags;
	u64 rsp;
	u64 ss;
} CpuState;

typedef struct Task {
	char name[TASK_NAME_MAX_LEN];

	enum { TASK_RUNNING, TASK_READY, TASK_WAITING, TASK_TERMINATED } state;

	CpuState regs;

	u32 priority;
	u32 ticks_remaining;

	struct Task *next;
} Task;

void task_init();

void task_create(struct Task *task, void (*entry_point)(void), const char *name, u32 priority);
void task_save_context(CpuState *regs);
void task_load_context(CpuState *regs, Task *task);
void task_switch();
struct task *task_current();
void task_exit();

void scheduler(CpuState *regs);

