#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 256
#define DEFAULT_STACK_SIZE 1024
#define STACK_END DEFAULT_STACK_SIZE - 1

typedef enum {
    TASK_RUNNING,
    TASK_READY,
    TASK_BLOCKED,
    TASK_TERMINATED
} task_state_t;

struct Task {
    void *kernel_stack_top;
    void *virtual_address_space;
    struct Task *next_task;
    int state;

    int scheduling_policy;
    int scheduling_priority;
    int pid;
    char task_name[64];              /* for debugging/monitoring purposes */
    unsigned long cpu_time_consumed; /* CPU time consumed by the task so far */
};

int create_task(struct task *task, void (*entry_point)(void));
void switch_task(struct task *current_task, struct task *next_task);
struct task *get_current_task();
void block_task(struct task *task);
void terminate_task(struct task *task);
void scheduler();
void initialize_task(struct task *task, void (*entry_point)(void));
void task_exit(void);
uint32_t get_task_id(struct task *task);



#endif