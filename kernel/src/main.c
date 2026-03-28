#include <helpers.h>
#include <init.h>
#include <kernelcfg.h>
#include <limine.h>
#include <pit.h>
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

    do_initcalls(__initcall0_start, __initcall0_end); /* early */
    do_initcalls(__initcall1_start, __initcall1_end); /* core */
    do_initcalls(__initcall2_start, __initcall2_end); /* postcore */
    do_initcalls(__initcall3_start, __initcall3_end); /* arch */
    do_initcalls(__initcall4_start, __initcall4_end); /* subsystem */
    do_initcalls(__initcall5_start, __initcall5_end); /* filesystem */
    do_initcalls(__initcall6_start, __initcall6_end); /* device */
    do_initcalls(__initcall7_start, __initcall7_end); /* late */

    yield();
}
