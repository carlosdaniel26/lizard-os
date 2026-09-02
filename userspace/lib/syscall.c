#include <sys/syscall.h>
#include <abi/syscall.h>
#include <dirent.h>

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

int sys_open(const char *path, int flags)
{
    return (int)__syscall2(SYS_open, path, flags);
}

int sys_close(int fd)
{
    return (int)__syscall1(SYS_close, fd);
}

long sys_lseek(int fd, long off, int whence)
{
    return __syscall3(SYS_lseek, fd, off, whence);
}

int sys_chdir(const char *path)
{
    return (int)__syscall1(SYS_chdir, path);
}

long sys_getcwd(char *buf, size_t size)
{
    return __syscall2(SYS_getcwd, buf, size);
}

int sys_mkdir(const char *path, int mode)
{
    return (int)__syscall2(SYS_mkdir, path, mode);
}

int sys_fb_info(struct fb_info *out)
{
    return (int)__syscall1(SYS_fb_info, out);
}

int sys_fb_blit(const void *xrgb, unsigned w, unsigned h)
{
    return (int)__syscall3(SYS_fb_blit, xrgb, w, h);
}

int sys_key_get(void)
{
    return (int)__syscall0(SYS_key_get);
}

unsigned long sys_uptime_ms(void)
{
    return (unsigned long)__syscall0(SYS_uptime_ms);
}

int sys_spawn(const char *path, char *const argv[])
{
    return (int)__syscall2(SYS_spawn, path, argv);
}

int sys_waitpid(int pid, int *status, int options)
{
    return (int)__syscall3(SYS_waitpid, pid, status, options);
}

int sys_yield(void)
{
    return (int)__syscall0(SYS_yield);
}

int sys_readdir(int fd, struct dirent *buf, int max)
{
    return (int)__syscall3(SYS_readdir, fd, buf, max);
}

__attribute__((noreturn)) void _exit(int code)
{
    __syscall1(SYS_exit, code);
    for (;;)
        ;
}
