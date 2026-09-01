#include <lizard/boot.h>
#include <lizard/helpers.h>
#include <lizard/init.h>
#include <lizard/kernelcfg.h>
#include <lizard/loader.h>
#include <lizard/pit.h>
#include <lizard/sched.h>
#include <nolibc/stdbool.h>
#include <lizard/task.h>
#include <nolibc/types.h>
#include <lizard/vfs.h>
#include <lizard/kmalloc.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <lizard/gdt.h>
#include <lizard/timer.h>

/*
 * feel dumb is temporary, the progress of commits on this
 * project is isn't so feel proud for every commit, even tho the problem is not solved.
 *
 * because its not solved yet. - Carlos, 03:46 30th December, 2025
 */

u8 kernel_stack[KERNEL_STACK_SIZE];

static struct task init_task;

/* pid 1-ish: keep a login shell alive. If /sh exits (or crashes) we just
 * start another one, so the machine never drops to a dead prompt. */
static void init_loop(void)
{
    for (;;)
    {
        int pid = spawn("/sh", 0);
        if (pid < 0)
        {
            kprintf("init: cannot spawn /sh - halting\n");
            task_exit();
        }

        int status = 0;
        task_waitpid(pid, &status);
        kprintf("init: /sh (pid %d) exited with %d, restarting\n", pid, status);
    }
}

/* Entered from head.S (_start_high) after the higher-half jump. head.S has
 * already switched RSP to the top of kernel_stack and published boot_info_ptr,
 * so - unlike under Limine - kmain must NOT move the stack itself: doing so
 * after the prologue set up %rbp leaves its locals in the dead gap above the
 * new RSP, where the next call stomps them. */
void kmain()
{
    boot_info_relocate();
    kernel_bootstrap();

    task_create(&init_task, init_loop, "init", 1, TASK_KERNEL);
    task_set_ready(&init_task);

    enable_scheduler();

    yield();
}
