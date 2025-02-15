#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 8
#define DEFAULT_STACK_SIZE 1024
#define STACK_END DEFAULT_STACK_SIZE - 1

struct Task
{
	void *kernel_stack_top;
	void *virtual_address_space;

	struct Task *next_task;
	int state; /* (running, waiting, ready)*/

	uint32_t eax, ebx, ecx, edx;
	uint32_t esi, edi, ebp;
	uint32_t esp;			   /* Stack pointer*/
	uint32_t eip;			   /* Instruction pointer*/
	uint32_t eflags;			/* Flags register*/

	/* Optional fields*/
	int scheduling_policy;  /* Task's scheduling policy*/
	int scheduling_priority;/* Task's scheduling priority*/
	int pid;				/* Process ID the task belongs to*/
	char task_name[64];		/* Task name (for debugging/monitoring purposes)*/
	uint32_t cpu_time_consumed; /* CPU time consumed by the task so far*/
};

int create_task(struct task *task, void (*entry_point)(void));
void switch_task(struct task *current_task, struct task *next_task);
struct task *get_current_task();
void block_task(struct task *task);
void terminate_task(struct task *task);
void scheduler();
void initialize_task(struct task *task, void (*entry_point)(void));
void task_exit(void);
uint32_t get_task_id(struct task *task);



#endif