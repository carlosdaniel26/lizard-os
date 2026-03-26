#pragma once

#include <types.h>

typedef int (*initcall_t)();

#define __initdata __attribute__((section(".init.data"), used))
#define __initconst __attribute__((section(".init.const"), used))

#define __initcall(fn, level, subl)                                                                          \
    initcall_t __initcall_##fn##level##subl                                                           \
        __attribute__((section(".initcall" #level ".sublevel" #subl "_" #fn ".init"), used)) = fn

// Standard initcall macros, defaulting to sublevel 00
#define core_initcall(fn) __initcall(fn, 1, 00)
#define postcore_initcall(fn) __initcall(fn, 2, 00)
#define arch_initcall(fn) __initcall(fn, 3, 00)
#define fs_initcall(fn) __initcall(fn, 5, 00)
#define device_initcall(fn) __initcall(fn, 6, 00)
#define late_initcall(fn) __initcall(fn, 7, 00)

// initcall with explicit sublevel
#define core_initcall_prio(fn, sublevel) __initcall(fn, 1, sublevel)
#define postcore_initcall_prio(fn, sublevel) __initcall(fn, 2, sublevel)
#define arch_initcall_prio(fn, sublevel) __initcall(fn, 3, sublevel)
#define fs_initcall_prio(fn, sublevel) __initcall(fn, 5, sublevel)
#define device_initcall_prio(fn, sublevel) __initcall(fn, 6, sublevel)
#define late_initcall_prio(fn, sublevel) __initcall(fn, 7, sublevel)

void do_initcalls();