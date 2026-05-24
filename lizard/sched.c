#include <lizard/idt.h>
#include <lizard/init.h>
#include <lizard/sched.h>
#include <lizard/task.h>
#include <nolibc/types.h>

#define SCHEDULER_ISR_INDEX 48

u8 scheduler_enabled = 0;

static int sched_init()
{
    isr_table[SCHEDULER_ISR_INDEX] = &isr_scheduler;

    task_create(&idle, &idle_func, "idle", 0, TASK_KERNEL); /* init idle */
    current_task = &idle;

    return 0;
}

subsys_initcall(sched_init);

void isr_scheduler(struct cpu_state *regs)
{
    scheduler(regs);
}

void scheduler()
{
    if (!scheduler_enabled) return;

    struct task *task = next_ready_task();

    /* if no ready task is found, default to idle */
    if (NULL == task)
    {
        task = &idle;
    }

    if (task != current_task)
    {
        task_switch_to(task);
        current_task = task;
    }
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
