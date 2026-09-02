#include "syscall.h"
#include <abi/errno.h>
#include <lizard/buddy.h>
#include <lizard/debug.h>
#include <lizard/gdt.h>
#include <lizard/helpers.h>
#include <lizard/init.h>
#include <lizard/ktime.h>
#include <lizard/pgtable.h>
#include <lizard/pit.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <lizard/task.h>
#include <lizard/tss.h>
#include <nolibc/types.h>
#include <lizard/vmm.h>

struct task idle = {0};

struct cpu_state *ptrace = {0};

LIST_HEAD(task_list);

struct task *current_task = NULL;

void idle_func()
{
    yield();
}

static u32 pid_counter;

void task_create(struct task *task, void (*entry_point)(void), const char *name, u32 priority, bool is_user)
{
    /* task->name = name */
    memset(task, 0, sizeof(struct task));
    memcpy(task->name, name, strlen(name));

    InitListHead(&task->children);
    InitListHead(&task->sibling);
    task->parent = NULL;
    task->wait_kind = WAIT_NONE;

    task->cwd[0] = '/';
    task->cwd[1] = '\0';

    task->pid = ++pid_counter;
    task->priority = priority;
    task->ticks_remaining = TASK_TIMESLICE;
    task->is_user = is_user;
    task->pml4 = pgtable_create();

    task->regs.rip = (u64)entry_point;
    task->regs.rflags = RFLAGS_DEFAULT;

    vaddr_t kstack = buddy_alloc(KSTACK_ORDER);
    memset((void *)kstack, 0, KSTACK_PAGES * PAGE_SIZE);
    task->kernel_stack = kstack + (KSTACK_PAGES * PAGE_SIZE);

    if (is_user) {
        task->regs.cs = USER_CS;
        task->regs.ss = USER_SS;

        u64 uflags = PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
        for (int i = 0; i < USER_STACK_PAGES; i++) {
            void *ptr = vmm_alloc(task->pml4, USER_STACK_BASE + (i * PAGE_SIZE), uflags);
            memset(ptr, 0, PAGE_SIZE);
        }
        task->regs.rsp = USER_STACK_BASE + (USER_STACK_PAGES * PAGE_SIZE);
    }
    else {
        task->regs.cs = KERNEL_CS;
        task->regs.ss = KERNEL_SS;
        task->regs.rsp = task->kernel_stack;
    }

    list_add(&task->list, &task_list);
    /* Not runnable yet: spawn() still has to load the image and lay out the
     * stack. The scheduler ignores TASK_STATE_NEW; the caller flips it to
     * READY once the task is safe to dispatch. */
    task->state = TASK_STATE_NEW;
}

void task_set_ready(struct task *task)
{
    if (task->state == TASK_STATE_NEW)
        task->state = TASK_STATE_READY;
}

void task_load_context(struct task *task)
{
    struct cpu_state *saved = &task->regs;

    ptrace->rax = saved->rax;
    ptrace->rdi = saved->rdi;
    ptrace->rsi = saved->rsi;
    ptrace->rdx = saved->rdx;
    ptrace->rcx = saved->rcx;
    ptrace->r8 = saved->r8;
    ptrace->r9 = saved->r9;
    ptrace->r10 = saved->r10;
    ptrace->r11 = saved->r11;
    ptrace->rbx = saved->rbx;
    ptrace->rbp = saved->rbp;
    ptrace->r12 = saved->r12;
    ptrace->r13 = saved->r13;
    ptrace->r14 = saved->r14;
    ptrace->r15 = saved->r15;

    ptrace->rip = saved->rip;
    ptrace->cs	 = saved->cs;
    ptrace->rflags = saved->rflags;
    ptrace->rsp = saved->rsp;
    ptrace->ss	 = saved->ss;
}

void task_save_context()
{
    struct cpu_state *saved = &current_task->regs;

    saved->rax = ptrace->rax;
    saved->rdi = ptrace->rdi;
    saved->rsi = ptrace->rsi;
    saved->rdx = ptrace->rdx;
    saved->rcx = ptrace->rcx;
    saved->r8 = ptrace->r8;
    saved->r9 = ptrace->r9;
    saved->r10 = ptrace->r10;
    saved->r11 = ptrace->r11;
    saved->rbx = ptrace->rbx;
    saved->rbp = ptrace->rbp;
    saved->r12 = ptrace->r12;
    saved->r13 = ptrace->r13;
    saved->r14 = ptrace->r14;
    saved->r15 = ptrace->r15;

    saved->rip = ptrace->rip;
    saved->cs  = ptrace->cs;
    saved->rflags = ptrace->rflags;
    saved->rsp = ptrace->rsp;
    saved->ss  = ptrace->ss;
}

static inline void scheduler_trigger()
{
    asm volatile("int $48");
}

void task_yield(void)
{
    scheduler_trigger();
}

void task_sleep(u32 ms)
{
    if (!current_task) return;

    current_task->sleep_until = pit_ticks + ms;
    current_task->wait_kind = WAIT_SLEEP;
    current_task->state = TASK_STATE_WAITING;
    scheduler_trigger();
}

void task_block(u8 wait_kind)
{
    if (!current_task || current_task == &idle) return;

    current_task->wait_kind = wait_kind;
    current_task->state = TASK_STATE_WAITING;
    scheduler_trigger();
    /* back here once task_wake() flipped us to READY and the scheduler
     * picked us again */
}

void task_wake(struct task *t)
{
    if (!t || t->state != TASK_STATE_WAITING) return;

    t->wait_kind = WAIT_NONE;
    t->state = TASK_STATE_READY;
}

int task_wake_all(u8 wait_kind)
{
    int n = 0;
    struct list_head *pos, *tmp;
    list_for_each(pos, tmp, &task_list)
    {
        struct task *t = container_of(pos, struct task, list);
        if (t->state == TASK_STATE_WAITING && t->wait_kind == wait_kind)
        {
            task_wake(t);
            n++;
        }
    }
    return n;
}

/*
 * Clean this code up to make sleep work properly
 */
int task_switch_to(struct task *next_task)
{
    task_save_context();
    task_load_context(next_task);

    if (next_task->pml4) {
        vmm_switch_pml4(next_task->pml4);
    }

    tss_set_stack(next_task->kernel_stack);

    current_task = next_task;

    return 0;
}

void task_tick()
{
    struct task *t = &idle;
    struct list_head *pos, *tmp;

    /* Wake up sleeping tasks. Only WAIT_SLEEP is timer-driven - a task parked
     * in WAIT_CHILD / WAIT_INPUT has sleep_until == 0 and must not be woken
     * here, only by task_wake(). */
    list_for_each(pos, tmp, &task_list)
    {
        t = (struct task *)pos;
        if (t->state == TASK_STATE_WAITING && t->wait_kind == WAIT_SLEEP &&
            pit_ticks >= t->sleep_until)
        {
            t->wait_kind = WAIT_NONE;
            t->state = TASK_STATE_READY;
        }
    }
}

struct task *next_ready_task()
{
    struct list_head *pos = current_task->list.next;

    /* task_list <--> .. <--> task <--> .. <--> task_list
     * so first we iterate the task to the right */
    while (pos != &task_list)
    {
        struct task *t = container_of(pos, struct task, list);
        if (t->state == TASK_STATE_READY && t != &idle) return t;
        pos = pos->next;
    }

    /* then from task_list to right until reach the task again*/
    pos = task_list.next;
    while (pos != &current_task->list)
    {
        struct task *t = container_of(pos, struct task, list);
        if (t->state == TASK_STATE_READY && t != &idle) return t;
        pos = pos->next;
    }

    return NULL;
}

void task_exit()
{
    struct task *me = current_task;

    if (me && me != &idle)
    {
        /* Re-parent any children onto idle (pid 1 / init). idle never calls
         * waitpid, so reap_terminated() frees an idle-owned corpse directly. */
        struct list_head *pos, *tmp;
        list_for_each(pos, tmp, &me->children)
        {
            struct task *c = container_of(pos, struct task, sibling);
            list_del(&c->sibling);
            list_add_tail(&c->sibling, &idle.children);
            c->parent = &idle;
        }

        me->state = TASK_STATE_TERMINATED;

        if (me->parent && me->parent != &idle)
        {
            /* a real parent may be parked in waitpid - let it collect us */
            if (me->parent->state == TASK_STATE_WAITING && me->parent->wait_kind == WAIT_CHILD)
                task_wake(me->parent);
        }
        else
        {
            me->reaped = true; /* nobody will wait - hand straight to the reaper */
        }
    }

    while (1)
    {
        scheduler_trigger();
    }
}

int task_waitpid(int pid, int *status)
{
    struct task *me = current_task;
    if (!me) return -EINVAL;

    for (;;)
    {
        struct list_head *pos, *tmp;
        struct task *zombie = NULL;
        int matches = 0;

        list_for_each(pos, tmp, &me->children)
        {
            struct task *c = container_of(pos, struct task, sibling);
            if (pid > 0 && (int)c->pid != pid) continue;
            matches++;
            if (c->state == TASK_STATE_TERMINATED)
            {
                zombie = c;
                break;
            }
        }

        if (matches == 0) return -ECHILD;

        if (zombie)
        {
            int rpid = (int)zombie->pid;
            if (status) *status = zombie->exit_code;
            list_del(&zombie->sibling);
            zombie->parent = NULL;
            zombie->reaped = true; /* reap_terminated() frees pml4 / kstack / struct */
            return rpid;
        }

        task_block(WAIT_CHILD); /* a child terminating wakes us via task_wake() */
    }
}
