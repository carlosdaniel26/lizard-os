#include <lizard/helpers.h>
#include <lizard/init.h>
#include <lizard/kernelcfg.h>
#include <lizard/limine.h>
#include <lizard/loader.h>
#include <lizard/pit.h>
#include <lizard/sched.h>
#include <lizard/stack.h>
#include <nolibc/stdbool.h>
#include <lizard/task.h>
#include <nolibc/types.h>
#include <lizard/vfs.h>
#include <lizard/kmalloc.h>
#include <nolibc/string.h>
#include <lizard/gdt.h>
#include <lizard/timer.h>

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
        task_create(t, (void (*)(void))0, "hello", 1, TASK_USER);
        load_elf(buffer, t);
    }
    
    enable_scheduler();

    yield();
}
