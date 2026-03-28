#include <idt.h>
#include <init.h>
#include <sched.h>
#include <task.h>
#include <types.h>

#define SCHEDULER_ISR_INDEX 48

u8 scheduler_enabled = 0;

static int sched_init()
{
    isr_table[SCHEDULER_ISR_INDEX] = &isr_scheduler;

    task_create(&idle, &idle_func, "idle", 0); /* init idle */
    current_task = &idle;

    return 0;
}

subsys_initcall(sched_init);

void isr_scheduler(CpuState *regs)
{
    scheduler(regs);
}

void scheduler()
{
    if (!scheduler_enabled) return;

    Task *task = next_ready_task();

    /* no ready task found */
    if (NULL == task)
    {
        if (current_task->state == TASK_STATE_READY)
        {
            return; /* able to keep running */
        }
        else
        {
            task = &idle; /* no task ready, fallback to idle */
        }
    }

    task_switch_to(task);
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
