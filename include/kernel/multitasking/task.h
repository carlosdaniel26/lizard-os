#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 8

#define MAX_PROCESS_NAME 64

enum task_state {
	TASK_READY = 0,	/* ready to be executed */
	TASK_RUNNING = 1,  /* currently being executed */
	TASK_WAITING = 2,  /* waiting for some event to occur */
	TASK_EXITED = 3	/* finished execution and is exiting */
};


struct task
{
	void *stack_top;
	void *virtual_address_space;

	struct task *prev_task;
	struct task *next_task;
	enum task_state state;

	uint32_t eax, ebx, ecx, edx;
	uint32_t esi, edi, ebp;
	uint32_t esp;				/* Stack pointer*/
	uint32_t eip;			  	/* Instruction pointer*/
	uint32_t eflags;			/* Flags register*/

	uint32_t ss;     			/* Stack Segment */
	uint32_t ds;     			/* Data Segment */
	uint32_t es;     			/* Extra Segment */
	uint32_t fs;     			/* Extra Segment */
	uint32_t gs;     			/* Extra Segment */

	/* Optional fields*/
	int scheduling_policy;  /* Task's scheduling policy*/
	int scheduling_priority;/* Task's scheduling priority*/
	int pid;				/* Process ID the task belongs to*/
	char name[MAX_PROCESS_NAME];		/* Task name (for debugging/monitoring purposes)*/
	uint32_t cpu_time_consumed; /* CPU time consumed by the task so far*/
};

int create_task(struct task *task, void (*entry_point)(void), const char p_name[]);
void save_task_context();
void jump_to_task();
void switch_task(struct task *current_task, struct task *next_task);
struct task *get_current_task();
void block_task(struct task *task);
void terminate_task(struct task *task);
void scheduler();
void initialize_task(struct task *task, void (*entry_point)(void));
void task_exit(void);
uint32_t get_task_id(struct task *task);
void kprint_task_state(struct task *t);
void init_tasks();



#endif