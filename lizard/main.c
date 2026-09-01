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

/* Entered from head.S (_start_high) after the higher-half jump. head.S has
 * already switched RSP to the top of kernel_stack and published boot_info_ptr,
 * so - unlike under Limine - kmain must NOT move the stack itself: doing so
 * after the prologue set up %rbp leaves its locals in the dead gap above the
 * new RSP, where the next call stomps them. */
void kmain()
{
    boot_info_relocate();
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
