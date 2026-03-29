#pragma once

#include <list.h>
#include <stdbool.h>
#include <types.h>

#define TASK_NAME_MAX_LEN 32

struct cpu_state {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rdi;
    u64 rsi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
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
} __attribute__((packed));

/* States */
#define TASK_STATE_RUNNING 0
#define TASK_STATE_READY 1
#define TASK_STATE_WAITING 2
#define TASK_STATE_TERMINATED 3

struct task {
    struct list_head list;
    char name[TASK_NAME_MAX_LEN];

    u8 state;

    struct cpu_state regs;

    u32 priority;
    u32 ticks_remaining;
    u32 sleep_until; /* Absolute wake-up time in ms */
};

void task_create(struct task *task, void (*entry_point)(void), const char *name, u32 priority);
void task_save_context();
void task_load_context(struct task *task);
void task_tick();
int task_switch();
struct task *task_current();
struct task *next_ready_task();
void task_exit();

void task_sleep(u32 ms);
void idle_func();

extern u8 scheduler_enabled;
extern struct task *current_task;
extern struct list_head task_list;

extern struct cpu_state *ptrace;
extern struct task idle;