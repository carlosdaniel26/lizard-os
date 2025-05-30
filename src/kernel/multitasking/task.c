#include <stdio.h>

#include <kernel/utils/helpers.h>
#include <kernel/multitasking/task.h>
#include <kernel/multitasking/pid.h>
#include <kernel/mem/pmm.h>
#include <kernel/mem/kmalloc.h>
#include <kernel/arch/i686/ptrace.h>
#include <kernel/shit-shell/ss.h>
#include <kernel/utils/alias.h>
#include <kernel/init.h>

struct task *task1 = NULL;

struct task *current_task = NULL;

extern void cpuid_kprint();

void kprint_task_state(struct task *t)
{
	kprintf("==== Task Name: %s\n ====", t->name);
	kprintf("State: ");
	switch (t->state) {
		case 0:
			kprintf("Ready\n");
			break;
		case 1:
			kprintf("Running\n");
			break;
		case 2:
			kprintf("Waiting\n");
			break;
		default:
			kprintf("Unknown\n");
			break;
	}
	kprintf("stack_top 0x%x\n", t->stack_top);
	kprintf("virtual_address_space 0x%x\n", t->virtual_address_space);
	kprintf("next_task 0x%x\n", t->next_task);
	kprintf("EIP: 0x%x\n", t->eip);
	kprintf("EAX: 0x%x\n", t->eax);
	kprintf("EBX: 0x%x\n", t->ebx);
	kprintf("ECX: 0x%x\n", t->ecx);
	kprintf("EDX: 0x%x\n", t->edx);
	kprintf("ESI: 0x%x\n", t->esi);
	kprintf("EDI: 0x%x\n", t->edi);
	kprintf("EBP: 0x%x\n", t->ebp);
	kprintf("ESP: 0x%x\n", t->esp);
	kprintf("EFLAGS: 0x%x\n", t->eflags);
	kprintf("Scheduling Policy: %u\n", t->scheduling_policy);
	kprintf("Scheduling Priority: %u\n", t->scheduling_priority);
	kprintf("PID: %u\n", t->pid);
	kprintf("CPU Time Consumed: %u\n", t->cpu_time_consumed);
}

static inline void clean_task(struct task *task)
{
	/* Clean Task*/
	task->stack_top = NULL;
	task->virtual_address_space = NULL;
	task->prev_task = NULL;
	task->next_task = NULL;
	task->state = 0;
	task->eax = 0;
	task->ebx = 0;
	task->ecx = 0;
	task->edx = 0;
	task->esi = 0;
	task->edi = 0;
	task->eflags = 0;
	task->scheduling_policy = 0;
	task->scheduling_priority = 0;
	task->pid = 0;
	for (int i = 0; i < MAX_PROCESS_NAME; i++) {
		task->name[i] = ' ';
	}
	task->cpu_time_consumed = 0;
}

int create_task(struct task *task, void (*entry_point)(void), const char p_name[])
{
	clean_task(task);

	/* Create State */
	for (int i = 0; i < MAX_PROCESS_NAME; i++)
		task->name[i] = p_name[i];
	
	task->pid = alloc_pid();

	uint32_t *stack = pmm_alloc_block(1);
	stack = align_ptr_down(stack, 16);

	task->esp = (uint32_t)stack;
	task->ebp = task->esp;

   	task->eip = (uint32_t)entry_point;

	return 1;
}

void task_exit()
{
	pmm_free_block((void*)current_task->esp);
	kfree(current_task);
	scheduler();
}

void init_tasks()
{
	/* Create PID 1 */
	task1 = (struct task *)kmalloc(sizeof(struct task));
	if (task1 == NULL) {
		kprintf("Error allocating PID1\n");
		return;
	}
	create_task(task1, (void *)init, "init");
	current_task = task1;
	jump_to_task(task1);
}

extern struct pt_regs ptrace;


void save_task_context()
{
	current_task->ebx = ptrace.ebx;
	current_task->ecx = ptrace.ecx;
	current_task->edx = ptrace.edx;
	current_task->esi = ptrace.esi;
	current_task->edi = ptrace.edi;
	current_task->ebp = ptrace.ebp;
	current_task->esp = ptrace.esp;
	current_task->eip = ptrace.eip;
	current_task->eflags = ptrace.eflags;
	current_task->eax = ptrace.eax;
}

void scheduler()
{
	save_task_context();

	struct task *next_task = current_task->next_task;
	if (next_task == NULL) {
		next_task = task1;
	}

	if (next_task == current_task) {
		return;
	}

	current_task = next_task;
	jump_to_task(next_task);
}
