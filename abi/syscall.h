#pragma once
/*
 * lizard syscall ABI - shared verbatim by the kernel (lizard/) and userspace
 * (userspace/). Numbers only; no types, no kernel or libc headers.
 *
 * Calling convention (int 0x80):
 *   RAX = syscall number
 *   RDI, RSI, RDX, R10, R8, R9 = args 1..6
 *   RAX = return value on the way out ( >= 0 ok, -errno on failure )
 */

enum {
    SYS_exit   = 0,
    SYS_write  = 1,
    SYS_read   = 2,
    SYS_sleep  = 3,
    SYS_getpid = 4,

    SYS_NR_MAX
};
