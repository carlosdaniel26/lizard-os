#include "syscall.h"
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
#include <task.h>
#include <types.h>
#include <vmm.h>

Task proc1 = {0};
Task idle = {0};

CpuState *ptrace = {0};

LIST_HEAD(task_list);

Task *current_task = NULL;

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

    current_task = &idle;

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
    list_add(&task->list, &task_list);

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
int task_switch_to(Task *next_task)
{
    task_save_context();
    task_load_context(next_task);

    current_task = next_task;

    return 0;
}

void task_tick()
{
    Task *t = &idle;
    ListHead *pos, *tmp;

    /* Wake up sleeping tasks */
    list_for_each(pos, tmp, &task_list)
    {
        t = (Task *)pos;
        if (t->state == TASK_STATE_WAITING && pit_ticks >= t->sleep_until)
        {
            t->state = TASK_STATE_READY;
        }
    }
}

Task *next_ready_task()
{
    ListHead *pos = current_task->list.next;

    /* task_list <--> .. <--> task <--> .. <--> task_list
     * so first we iterate the task to the right */
    while (pos != &task_list)
    {
        Task *t = container_of(pos, Task, list);
        if (t->state == TASK_STATE_READY && t != &idle) return t;
        pos = pos->next;
    }

    /* then from task_list to right until reach the task again*/
    pos = task_list.next;
    while (pos != &current_task->list)
    {
        Task *t = container_of(pos, Task, list);
        if (t->state == TASK_STATE_READY && t != &idle) return t;
        pos = pos->next;
    }

    return NULL;
}
