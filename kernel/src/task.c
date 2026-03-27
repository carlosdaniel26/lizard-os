#include <buddy.h>
#include <debug.h>
#include <helpers.h>
#include <init.h>
#include <ktime.h>
#include <pgtable.h>
#include <pit.h>
#include <ss.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <task.h>
#include <types.h>
#include <vmm.h>

static Task proc1 = {0};
static Task idle = {0};
Task *current_task = &idle;
CpuState *ptrace = {0};

u8 scheduler_enabled = 0;

void idle_func()
{
    while (1)
    {
        hlt();
    }
}

void proc1_func()
{
    int i = 0;
    while (1)
    {
        kprintf("hello %u", i++);
        syscall1(0, 1000 * 2); // Syscall 0 is sys_sleep
    }
    hlt();
}

int task_init()
{
    task_create(&idle, &idle_func, "idle", 1);
    task_create(&proc1, &proc1_func, "proc1", 1);

    return 0;
}

subsys_initcall(task_init);

void task_create(struct Task *task, void (*entry_point)(void), const char *name, u32 priority)
{
    /* task->name = name */
    memcpy(task->name, name, strlen(name));

    task->priority = priority;
    task->regs.rip = (u64)entry_point;
    uintptr_t ptr = (uintptr_t)vmm_alloc_page();

    memset((void *)ptr, 0, 4096);

    task->regs.rsp = ptr + 4096;
    Task *i;
    for (i = current_task; i->next != NULL; i = i->next)
        ;

    i->next = task;
    task->next = NULL;

    task->state = TASK_STATE_READY;
}

void task_load_context(Task *task)
{
    CpuState *saved = &task->regs;

    ptrace->rax = saved->rax;
    ptrace->rdi = saved->rdi;
    ptrace->rsi = saved->rsi;
    ptrace->rdx = saved->rdx;
    ptrace->rcx = saved->rcx;
    ptrace->r8 = saved->r8;
    ptrace->r9 = saved->r9;
    ptrace->r10 = saved->r10;
    ptrace->r11 = saved->r11;
    ptrace->rbx = saved->rbx;
    // ptrace->rbp = saved->rbp;
    ptrace->r12 = saved->r12;
    ptrace->r13 = saved->r13;
    ptrace->r14 = saved->r14;
    ptrace->r15 = saved->r15;

    ptrace->rip = saved->rip;
    // ptrace->cs	 = saved->cs;
    // ptrace->rflags = saved->rflags;
    ptrace->rsp = saved->rsp;
    // ptrace->ss	 = saved->ss;
}

void task_save_context()
{
    CpuState *saved = &current_task->regs;

    saved->rax = ptrace->rax;
    saved->rdi = ptrace->rdi;
    saved->rsi = ptrace->rsi;
    saved->rdx = ptrace->rdx;
    saved->rcx = ptrace->rcx;
    saved->r8 = ptrace->r8;
    saved->r9 = ptrace->r9;
    saved->r10 = ptrace->r10;
    saved->r11 = ptrace->r11;
    saved->rbx = ptrace->rbx;
    saved->rbp = ptrace->rbp;
    saved->r12 = ptrace->r12;
    saved->r13 = ptrace->r13;
    saved->r14 = ptrace->r14;
    saved->r15 = ptrace->r15;

    saved->rip = ptrace->rip;
    // saved->cs  = ptrace->cs;
    // saved->rflags = ptrace->rflags;
    saved->rsp = ptrace->rsp;
    // saved->ss  = ptrace->ss;
}

static inline void scheduler_trigger()
{
    asm volatile("int $48");
}

void task_sleep(u32 ms)
{
    if (!current_task) return;

    current_task->sleep_until = pit_ticks + ms;
    current_task->state = TASK_STATE_WAITING;
    scheduler_trigger();
}

/*
 * Clean this code up to make sleep work properly
 */
int task_switch()
{
    Task *old_task = current_task;

    if (old_task->state == TASK_STATE_RUNNING) old_task->state = TASK_STATE_READY;

    Task *next_task = current_task->next;

    if (next_task == NULL) next_task = &idle;

    /* Find the next READY task */
    while (next_task->state != TASK_STATE_READY)
    {
        next_task = next_task->next;

        if (next_task == NULL) next_task = &idle;
    }

    if (next_task == old_task)
    {
        next_task->state = TASK_STATE_RUNNING;
        return 0;
    }

    task_save_context();
    task_load_context(next_task);

    next_task->state = TASK_STATE_RUNNING;

    current_task = next_task;

    ptrace->rip = current_task->regs.rip;
    ptrace->rsp = current_task->regs.rsp;

    return 0;
}

void task_tick()
{
    Task *t = &idle;

    while (t)
    {
        if (t->state == TASK_STATE_WAITING && pit_ticks >= t->sleep_until)
        {
            t->state = TASK_STATE_READY;
        }
        t = t->next;
    }
}

void scheduler()
{
    if (!scheduler_enabled) return;

    task_switch();
}

void enable_scheduler()
{
    scheduler_enabled = 1;
}

late_initcall(enable_scheduler);

void disable_scheduler()
{
    scheduler_enabled = 0;
}

u8 scheduler_is_enabled()
{
    return scheduler_enabled;
}