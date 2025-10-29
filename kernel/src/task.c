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

void idle_func()
{
	while (1)
	{
		hlt();
	}
}

void proc1_func()
{
	shit_shell_init();
	hlt();
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

static inline void scheduler_trigger()
{
    asm volatile ("int $32"); // 32 is your PIT IRQ
}

void task_sleep(u32 ms)
{
	if (!current_task)
		return;

	current_task->sleep_ticks = ms;
	current_task->state = TASK_STATE_WAITING;
}

/*
 * Clean this code up to make sleep work properly
*/
int task_switch(CpuState *prev_regs)
{
	Task *next_task = current_task->next;

	if (next_task == NULL)
		next_task = &idle;

	/* Find the next READY task */
	while (next_task->state != TASK_STATE_READY)
	{
		next_task = next_task->next;

		if (next_task == NULL)
			next_task = &idle;
	}

	if (next_task == current_task)
		return 0;

	task_save_context(prev_regs);
	task_load_context(prev_regs, next_task);

	current_task->state = TASK_STATE_READY;
	next_task->state = TASK_STATE_RUNNING;

	current_task = next_task;

	prev_regs->rip = current_task->regs.rip;
	prev_regs->rsp = current_task->regs.rsp;

	return 0;
}

void scheduler(CpuState *regs)
{
	if (! scheduler_enabled)
		return;

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