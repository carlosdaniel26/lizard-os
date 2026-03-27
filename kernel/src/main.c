#include <helpers.h>
#include <init.h>
#include <kernelcfg.h>
#include <limine.h>
#include <stack.h>
#include <stdbool.h>
#include <types.h>

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
    do_initcalls();
    hlt();
}
