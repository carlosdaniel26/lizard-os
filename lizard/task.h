#pragma once

#include <nolibc/list.h>
#include <nolibc/stdbool.h>
#include <nolibc/types.h>

#define TASK_NAME_MAX_LEN 32

/* Longest absolute path a task's cwd may hold, terminator included. */
#define TASK_CWD_MAX 128

struct file; /* lizard/file.h - tasks only hold pointers */

#define USER_STACK_BASE  0x700000000000
#define USER_STACK_PAGES 64 /* 256 KiB - doomgeneric's BSP recursion wants room */

/* Per-task open files. 0/1/2 are the tty and are not stored here; real
 * descriptors start at 3 and index fd_table[fd - 3]. */
#define TASK_FD_BASE 3
#define TASK_MAX_FDS 16

#define KSTACK_ORDER 2
#define KSTACK_PAGES (1 << KSTACK_ORDER)

#define RFLAGS_DEFAULT 0x202

/* Round-robin quantum, in timer ticks (~ms at the 1 kHz PIT). */
#define TASK_TIMESLICE 5

#define TASK_USER   true
#define TASK_KERNEL false

struct cpu_state {
    u64 rax;
    u64 rbx;
    u64 rcx;
    u64 rdx;
    u64 rdi;
    u64 rsi;
    u64 rbp;
    u64 r8;
    u64 r9;
    u64 r10;
    u64 r11;
    u64 r12;
    u64 r13;
    u64 r14;
    u64 r15;

    u64 vec;     /* interrupt / exception vector, pushed by the per-vector stub */
    u64 errcode; /* CPU error code for #GP/#PF/... ; 0 for everything else      */

    /* Interrupt Frame */
    u64 rip;
    u64 cs;
    u64 rflags;
    u64 rsp;
    u64 ss;
} __attribute__((packed));

/* States */
#define TASK_STATE_RUNNING 0
#define TASK_STATE_READY 1
#define TASK_STATE_WAITING 2
#define TASK_STATE_TERMINATED 3 /* also the zombie state: dead, not yet waited on */
#define TASK_STATE_NEW 4        /* created but not yet runnable (image still loading) */

/* Why a WAITING task is off the run queue. task_tick() only auto-wakes
 * WAIT_SLEEP; the rest are woken explicitly by task_wake(). */
enum {
    WAIT_NONE = 0,
    WAIT_SLEEP,  /* task_sleep() - wake when pit_ticks >= sleep_until */
    WAIT_CHILD,  /* sys_waitpid() - wake when a child terminates      */
    WAIT_INPUT,  /* blocked in sys_read() on the tty - wake on a line  */
};

struct task {
    struct list_head list;
    char name[TASK_NAME_MAX_LEN];

    u32 pid;
    int exit_code;

    u8 state;
    u8 wait_kind; /* WAIT_* - valid while state == TASK_STATE_WAITING */
    bool is_user;
    bool on_heap; /* task struct was kmalloc'd - reaper should kfree it */
    bool reaped;  /* zombie already collected by waitpid - reaper may free it */

    struct cpu_state regs;
    vaddr_t pml4;

    u64 kernel_stack;

    /* Process tree. parent == NULL means orphaned (or never parented); the
     * reaper frees such a task's corpse without anyone waiting. children is
     * this task's list head, sibling is its link in parent->children. */
    struct task *parent;
    struct list_head children;
    struct list_head sibling;

    u32 priority;
    u32 ticks_remaining;
    u32 sleep_until; /* Absolute wake-up time in ms */

    struct file *fd_table[TASK_MAX_FDS]; /* NULL = free slot; see TASK_FD_BASE */

    /* Current working directory: always an absolute, lexically-normalised path
     * ("/" for a fresh task). spawn() copies the parent's down to the child. */
    char cwd[TASK_CWD_MAX];
};

void task_create(struct task *task, void (*entry_point)(void), const char *name, u32 priority, bool is_user);
void task_set_ready(struct task *task); /* promote a TASK_STATE_NEW task onto the run queue */
void task_save_context();
void task_load_context(struct task *task);
int task_switch_to(struct task *next_task);
void task_tick();
int task_switch();
struct task *task_current();
struct task *next_ready_task();
void task_exit();

void task_sleep(u32 ms);
void task_yield(void); /* voluntary reschedule (int 48) */

/* Block the current task off the run queue with the given WAIT_* reason, then
 * reschedule. Returns once someone has moved it back to READY. */
void task_block(u8 wait_kind);
/* Move a WAITING task back to READY (no-op if it is not WAITING). */
void task_wake(struct task *t);
/* Wake every task blocked with this wait_kind; returns how many. */
int task_wake_all(u8 wait_kind);

/* Wait for a child to terminate. pid > 0 waits for that pid, pid <= 0 for any
 * child. On success returns the reaped child's pid and, if status != NULL,
 * stores its exit code there; -ECHILD if there are no matching children. */
int task_waitpid(int pid, int *status);

void idle_func();

extern u8 scheduler_enabled;
extern struct task *current_task;
extern struct list_head task_list;

extern struct cpu_state *ptrace;
extern struct task idle;