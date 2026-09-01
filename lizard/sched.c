#include <lizard/buddy.h>
#include <lizard/idt.h>
#include <lizard/init.h>
#include <lizard/kmalloc.h>
#include <lizard/pgtable.h>
#include <lizard/sched.h>
#include <lizard/task.h>
#include <nolibc/stddef.h>
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

/* Free the kernel resources of any TERMINATED task that is no longer the one
 * we are running on. A TERMINATED task with a live real parent is a zombie: it
 * is left alone until sys_waitpid() collects its exit code (which sets reaped).
 * Corpses owned by idle (pid 1) have nobody to wait on them, so they go too. */
static void reap_terminated(void)
{
    struct list_head *pos, *tmp;
    list_for_each(pos, tmp, &task_list)
    {
        struct task *t = container_of(pos, struct task, list);
        if (t == &idle || t == current_task || t->state != TASK_STATE_TERMINATED)
            continue;
        if (!t->reaped && t->parent && t->parent != &idle)
            continue; /* zombie - waitpid() has not collected it yet */

        list_del(&t->list);
        if (t->sibling.next)
            list_del(&t->sibling);
        if (t->kernel_stack)
            buddy_free(t->kernel_stack - (KSTACK_PAGES * PAGE_SIZE), KSTACK_ORDER);
        if (t->pml4)
            pgtable_free_tree(t->pml4);
        if (t->on_heap)
            kfree(t);
    }
}

/*
 * Round-robin. Called every PIT tick and on every voluntary yield (int 48).
 * A RUNNING task keeps the CPU until its quantum (ticks_remaining) drains;
 * then the next READY task in list order gets a fresh quantum. idle is only
 * chosen when nothing else is runnable.
 */
void scheduler()
{
    if (!scheduler_enabled) return;

    reap_terminated();

    struct task *cur = current_task;
    bool cur_runnable = cur && cur->state == TASK_STATE_RUNNING;

    /* Spend one tick of the current quantum. Voluntary entries (sleep/exit
     * have already moved the state to WAITING/TERMINATED) skip this and
     * reschedule immediately. */
    if (cur_runnable && cur != &idle)
    {
        if (cur->ticks_remaining)
            cur->ticks_remaining--;
        if (cur->ticks_remaining)
            return; /* quantum not up - keep running cur */
    }

    struct task *next = next_ready_task(); /* rotates from cur->list.next */

    if (!next)
    {
        if (cur_runnable)
        {
            cur->ticks_remaining = TASK_TIMESLICE; /* sole runnable task */
            return;
        }
        next = &idle; /* cur is blocked/dead and nobody is ready */
    }

    if (cur_runnable && cur != &idle)
        cur->state = TASK_STATE_READY; /* put cur back in the rotation */

    next->state = TASK_STATE_RUNNING;
    next->ticks_remaining = TASK_TIMESLICE;

    if (next != cur)
        task_switch_to(next); /* also sets current_task */
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
