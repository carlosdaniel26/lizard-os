#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <task.h>
#include <helpers.h>

Task proc1;

void proc1_func()
{
	kprintf("proc1\n");
	hlt();
}

void task_init()
{
	task_create(&proc1, &proc1_func, "proc1", 1);
	task_load_context(&proc1);
}

void task_create(struct Task *task, void (*entry_point)(void), const char *name, uint32_t priority)
{
	/* task->name = name */
	memcpy(task->name, name, strlen(name));

	task->priority = priority;
	task->regs.rip = (uint64_t)entry_point;
}