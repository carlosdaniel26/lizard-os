#include <helpers.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <pmm.h>

Task *current_task;
Task proc1;

void proc1_func()
{
    hlt();
}

void task_init()
{
    task_create(&proc1, &proc1_func, "proc1", 1);
}

void task_create(struct Task *task, void (*entry_point)(void), const char *name, uint32_t priority)
{
    /* task->name = name */
    memcpy(task->name, name, strlen(name));

    task->priority = priority;
    task->regs.rip = (uint64_t)entry_point;

    uint64_t ptr = (uint64_t)pmm_alloc_block();
    memset((void *)ptr, 0, 4096);

    task->regs.rsp = ptr + 4096;
}

void task_load_context(CpuState *regs, Task *task)
{
    CpuState *saved = &task->regs;

    regs->rax = saved->rax;
    regs->rdi = saved->rdi;
    regs->rsi = saved->rsi;
    regs->rdx = saved->rdx;
    regs->rcx = saved->rcx;
    regs->r8 = saved->r8;
    regs->r9 = saved->r9;
    regs->r10 = saved->r10;
    regs->r11 = saved->r11;
    regs->rbx = saved->rbx;
    // regs->rbp = saved->rbp;
    regs->r12 = saved->r12;
    regs->r13 = saved->r13;
    regs->r14 = saved->r14;
    regs->r15 = saved->r15;

    regs->rip = saved->rip;
    // regs->cs  = saved->cs;
    // regs->rflags = saved->rflags;
    regs->rsp = saved->rsp;
    // regs->ss  = saved->ss;
}

void task_save_context(CpuState *regs)
{
    CpuState *saved = &current_task->regs;

    saved->rax = regs->rax;
    saved->rdi = regs->rdi;
    saved->rsi = regs->rsi;
    saved->rdx = regs->rdx;
    saved->rcx = regs->rcx;
    saved->r8 = regs->r8;
    saved->r9 = regs->r9;
    saved->r10 = regs->r10;
    saved->r11 = regs->r11;
    saved->rbx = regs->rbx;
    saved->rbp = regs->rbp;
    saved->r12 = regs->r12;
    saved->r13 = regs->r13;
    saved->r14 = regs->r14;
    saved->r15 = regs->r15;

    saved->rip = regs->rip;
    // saved->cs  = regs->cs;
    // saved->rflags = regs->rflags;
    saved->rsp = regs->rsp;
    // saved->ss  = regs->ss;
}

void scheduler(CpuState *regs)
{
    if (!current_task)
    {
        current_task = &proc1;
        task_load_context(regs, current_task);
        return;
    }

    task_save_context(regs);

    Task *next_task = current_task->next;
    if (next_task == NULL)
    {
        next_task = &proc1;
    }

    if (next_task == current_task)
    {
        return;
    }

    task_load_context(regs, next_task);
    current_task = next_task;

    regs->rip = current_task->regs.rip;
    regs->rsp = current_task->regs.rsp;
}