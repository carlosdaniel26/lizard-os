#include <helpers.h>
#include <types.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <pmm.h>
#include <ss.h>
#include <clock.h>

static Task proc1 = {0};
static Task idle = {0};
static Task *current_task = &idle;

u8 scheduler_enabled = 0;

static inline void save_current_regs(CpuState *regs)
{
    asm volatile(
        "mov %%rax, %0\n"
        "mov %%rbx, %1\n"
        "mov %%rcx, %2\n"
        "mov %%rdx, %3\n"
        "mov %%rsi, %4\n"
        "mov %%rdi, %5\n"
        "mov %%rbp, %6\n"
        "mov %%r8,  %7\n"
        "mov %%r9,  %8\n"
        "mov %%r10, %9\n"
        "mov %%r11, %10\n"
        "mov %%r12, %11\n"
        "mov %%r13, %12\n"
        "mov %%r14, %13\n"
        "mov %%r15, %14\n"
        : "=m"(regs->rax), "=m"(regs->rbx), "=m"(regs->rcx), "=m"(regs->rdx),
          "=m"(regs->rsi), "=m"(regs->rdi), "=m"(regs->rbp), "=m"(regs->r8),
          "=m"(regs->r9), "=m"(regs->r10), "=m"(regs->r11), "=m"(regs->r12),
          "=m"(regs->r13), "=m"(regs->r14), "=m"(regs->r15)
    );
    regs->rsp = (u64)__builtin_frame_address(0);
    regs->rip = (u64)&&resume_here;
resume_here:;
}

void idle_func()
{
	while (1)
	{
		hlt();
	}
}

void proc1_func()
{
    
    while (1) {
        kprintf("Proc1: Before sleep\n");
        task_sleep(1000);
        kprintf("Proc1: After sleep - Woke up!\n");
    }
}

void task_init()
{
	task_create(&idle, &idle_func, "idle", 1);
	task_create(&proc1, &proc1_func, "proc1", 1);
}

void task_create(struct Task *task, void (*entry_point)(void), const char *name, u32 priority)
{
	/* task->name = name */
	memcpy(task->name, name, strlen(name));

	task->priority = priority;
	task->regs.rip = (u64)entry_point;

	u64 ptr = (u64)pmm_alloc_block() + hhdm_offset;
	memset((void *)ptr, 0, 4096);

	task->regs.rsp = ptr + 4096;
	Task *i;
	for (i = current_task; i->next != NULL; i = i->next);

	i->next = task;
	task->next = NULL;

	task->state = TASK_STATE_READY;
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
	// regs->cs	 = saved->cs;
	// regs->rflags = saved->rflags;
	regs->rsp = saved->rsp;
	// regs->ss	 = saved->ss;
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

static inline void timer_tick()
{
    Task *t = &idle;
    int woke_up = 0;
    
    do {
        if (t->state == TASK_STATE_WAITING && t->sleep_ticks > 0) {
            t->sleep_ticks--;
            if (t->sleep_ticks == 0) {
                t->state = TASK_STATE_READY;
                woke_up++;
            }
        }
        t = t->next;
    } while (t != NULL);
    
    if (woke_up > 0) {
    }
}

void task_sleep(u32 ms)
{
    if (!current_task || current_task == &idle) {
        return;
    }

    save_current_regs(&current_task->regs);
    
    current_task->sleep_ticks = ms;
    current_task->state = TASK_STATE_WAITING;
    
    task_switch(&current_task->regs);
}

static Task* choose_next_task(void)
{
    Task *next_task = current_task->next;
    if (next_task == NULL) {
        next_task = &idle;
    }

    Task *start_task = next_task;
    int searches = 0;
    const int MAX_SEARCHES = 10;

    while (next_task->state != TASK_STATE_READY)
	{
        searches++;
        if (searches >= MAX_SEARCHES) {
            next_task = &idle;
            break;
        }
        
        next_task = next_task->next;
        if (next_task == NULL) {
            next_task = &idle;
        }
        
        if (next_task == start_task) {
            next_task = &idle;
            break;
        }
    }

    if (next_task->state != TASK_STATE_READY)
{
        next_task = &idle;
        next_task->state = TASK_STATE_READY;
    }

    return next_task;
}

int task_switch(CpuState *prev_regs)
{
    if (!scheduler_enabled) {
        return 0;
    }

    Task *next_task = choose_next_task();

    if (next_task == current_task && current_task != &idle) {
        return 0;
    }

    if (current_task != &idle) {
        task_save_context(prev_regs);
    }

    if (current_task->state == TASK_STATE_RUNNING && current_task != &idle) {
        current_task->state = TASK_STATE_READY;
    }
    
    next_task->state = TASK_STATE_RUNNING;

    task_load_context(prev_regs, next_task);

    current_task = next_task;
    
    return 1;
}

void scheduler(CpuState *regs)
{
    if (!scheduler_enabled) {
        return;
    }

    timer_tick();
    
	task_switch(regs);
}

void enable_scheduler()
{
	scheduler_enabled = 1;
}

void disable_scheduler()
{
	scheduler_enabled = 0;
}

u8 scheduler_is_enabled()
{
	return scheduler_enabled;
}