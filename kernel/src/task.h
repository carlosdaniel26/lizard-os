#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define TASK_NAME_MAX_LEN 32

typedef struct CpuState {
    uint64_t rax;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t rbx;
    uint64_t rbp;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;

    /* Interrupt Frame */
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} CpuState;

typedef struct  __attribute__((packed)) Task
{
	char name[TASK_NAME_MAX_LEN];

	enum {
		TASK_RUNNING,
		TASK_READY,
		TASK_WAITING,
		TASK_TERMINATED
	} state;

	CpuState regs;

	uint32_t priority;
	uint32_t ticks_remaining;

	struct Task *next;
} Task;

void task_init();

void task_create(struct Task *task, void (*entry_point)(void), const char *name, uint32_t priority);
void task_save_context(CpuState *regs);
void task_load_context(CpuState *regs, Task *task);
void task_switch();
struct task *task_current();
void task_exit();

void scheduler(CpuState *regs);

#endif