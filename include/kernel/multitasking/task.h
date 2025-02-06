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

struct task {
    uint8_t *stack;
    uint32_t esp;
    uint32_t ebp;
    uint32_t eip;

    task_state_t state;
    uint32_t id;
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