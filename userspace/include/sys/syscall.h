#pragma once
#include <abi/syscall.h>
#include <abi/errno.h>
#include <stddef.h>

/* Raw syscall - nr in RAX, args in RDI RSI RDX R10 R8 R9, result in RAX. */
long __syscall6(long nr, long a1, long a2, long a3, long a4, long a5, long a6);

#define __syscall0(n)                 __syscall6((n), 0, 0, 0, 0, 0, 0)
#define __syscall1(n, a)              __syscall6((n), (long)(a), 0, 0, 0, 0, 0)
#define __syscall2(n, a, b)           __syscall6((n), (long)(a), (long)(b), 0, 0, 0, 0)
#define __syscall3(n, a, b, c)        __syscall6((n), (long)(a), (long)(b), (long)(c), 0, 0, 0)

/* Typed wrappers. Return >= 0 on success, -errno on failure. */
long sys_write(int fd, const void *buf, size_t len);
long sys_read(int fd, void *buf, size_t len);
int  sys_sleep(unsigned ms);
int  sys_getpid(void);

__attribute__((noreturn)) void _exit(int code);
