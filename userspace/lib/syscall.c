#include <sys/syscall.h>

long __syscall6(long n, long a, long b, long c, long d, long e, long f)
{
    long ret;
    register long r10 __asm__("r10") = d;
    register long r8  __asm__("r8")  = e;
    register long r9  __asm__("r9")  = f;

    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(n), "D"(a), "S"(b), "d"(c),
                       "r"(r10), "r"(r8), "r"(r9)
                     : "memory", "cc");
    return ret;
}

long sys_write(int fd, const void *buf, size_t len)
{
    return __syscall3(SYS_write, fd, buf, len);
}

long sys_read(int fd, void *buf, size_t len)
{
    return __syscall3(SYS_read, fd, buf, len);
}

int sys_sleep(unsigned ms)
{
    return (int)__syscall1(SYS_sleep, ms);
}

int sys_getpid(void)
{
    return (int)__syscall0(SYS_getpid);
}

__attribute__((noreturn)) void _exit(int code)
{
    __syscall1(SYS_exit, code);
    for (;;)
        ;
}
