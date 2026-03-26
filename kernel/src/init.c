#include <init.h>

void do_initcalls()
{
    extern const initcall_t __initcall_start[];
    extern const initcall_t __initcall_end[];

    for (const initcall_t *fn = &__initcall_start; fn < &__initcall_end; fn++)
    {
        (*fn)();
    }
}