#include <init.h>

void do_initcalls(initcall_t *start, initcall_t *end)
{
    for (const initcall_t *fn = start; fn < end; fn++)
    {
        (*fn)();
    }
}
