#include <helpers.h>
#include <init.h>
#include <kernelcfg.h>
#include <limine.h>
#include <loader.h>
#include <pit.h>
#include <sched.h>
#include <stack.h>
#include <stdbool.h>
#include <task.h>
#include <types.h>
#include <vfs.h>
#include <kmalloc.h>
#include <string.h>
#include <gdt.h>

/*
 * feel dumb is temporary, the progress of commits on this
 * project is isn't so feel proud for every commit, even tho the problem is not solved.
 *
 * because its not solved yet. - Carlos, 03:46 30th December, 2025
 */

__attribute__((used, section(".limine_requests"))) static volatile LIMINE_BASE_REVISION(3);
__attribute__((used, section(".limine_requests_start"))) static volatile LIMINE_REQUESTS_START_MARKER;
__attribute__((used, section(".limine_requests_end"))) static volatile LIMINE_REQUESTS_END_MARKER;

u8 kernel_stack[KERNEL_STACK_SIZE];

void kmain()
{
    stack_init(kernel_stack, KERNEL_STACK_SIZE);
    kernel_bootstrap();

    /* Test loading */
    struct task *t = (struct task *)zalloc(sizeof(struct task));
    void *buffer = vfs_read_all("/hello");
    if (buffer) {
        load_elf(buffer, t);
        task_create(t, (void (*)(void))t->regs.rip, "hello", 1);
    }

    pit_start();
    enable_scheduler();

    yield();
}
