#include <stdio.h>

#include <kernel/multitasking/task.h>
#include <kernel/multitasking/pid.h>
#include <kernel/mem/pmm.h>

struct task *tasks;

extern void cpuid_kprint();

void kprint_task_state(struct task *t)
{
	kprintf("Task Name: %s\n", t->name);
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
	kprintf("cpuid_kprint: 0x%x\n", &cpuid_kprint);
	if (t->eip == (uint32_t)&cpuid_kprint)
	{
		kprintf("SAME\n");
	}
	else
	{
		kprintf("N SAME\n");
	}
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

int create_task(struct task *task, void (*entry_point)(void), const char p_name[])
{
	/* Clean Task*/
	task->stack_top = NULL;
	task->virtual_address_space = NULL;
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
		task->name[i] = p_name[i];
	}
	task->cpu_time_consumed = 0;

	/* Create State */

	task->pid = alloc_pid();

	uint8_t *stack_block = pmm_alloc_block();
	uint8_t *stack_top = stack_block + 4095;



   	task->eip = (uint32_t)entry_point;
	task->esp = (uint32_t)stack_top;
	task->ebp = (uint32_t)stack_top;

	return 1;
}