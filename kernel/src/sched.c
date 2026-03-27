#include <init.h>
#include <sched.h>
#include <task.h>
#include <types.h>

u8 scheduler_enabled = 0;

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

late_initcall(enable_scheduler);

void disable_scheduler()
{
    scheduler_enabled = 0;
}

u8 scheduler_is_enabled()
{
    return scheduler_enabled;
}