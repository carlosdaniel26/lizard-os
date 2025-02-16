#include <stdio.h>

#include <kernel/multitasking/task.h>

struct task *tasks;

void print_task_state(struct task *t) 
{
    printf("Task Name: %s\n", t->name);
    printf("State: ");
    switch (t->state) {
        case 0:
            printf("Ready\n");
            break;
        case 1:
            printf("Running\n");
            break;
        case 2:
            printf("Waiting\n");
            break;
        default:
            printf("Unknown\n");
            break;
    }
    printf("EIP: %x\n", t->eip);
    printf("EAX: %x\n", t->eax);
    printf("EBX: %x\n", t->ebx);
    printf("ECX: %x\n", t->ecx);
    printf("EDX: %x\n", t->edx);
    printf("ESI: %x\n", t->esi);
    printf("EDI: %x\n", t->edi);
    printf("EBP: %x\n", t->ebp);
    printf("ESP: %x\n", t->esp);
    printf("EFLAGS: %x\n", t->eflags);
    printf("Scheduling Policy: %u\n", t->scheduling_policy);
    printf("Scheduling Priority: %u\n", t->scheduling_priority);
    printf("PID: %u\n", t->pid);
    printf("CPU Time Consumed: %u\n", t->cpu_time_consumed);
}

void clean_tasks_state(struct task *task) 
{
    task->kernel_stack_top = NULL;
    task->virtual_address_space = NULL;
    task->next_task = NULL;
    task->state = 0;
    task->eax = 0;
    task->ebx = 0;
    task->ecx = 0;
    task->edx = 0;
    task->esi = 0;
    task->edi = 0;
    task->ebp = 0;
    task->esp = 0;
    task->eip = 0;
    task->eflags = 0;
    task->scheduling_policy = 0;
    task->scheduling_priority = 0;
    task->pid = 0;
    for (int i = 0; i < 64; i++) {
        task->name[i] = 0;
    }
    task->cpu_time_consumed = 0;
}
