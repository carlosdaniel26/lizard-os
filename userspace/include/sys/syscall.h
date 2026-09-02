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

int  sys_open(const char *path, int flags);
int  sys_close(int fd);
long sys_lseek(int fd, long off, int whence);
int  sys_chdir(const char *path);
long sys_getcwd(char *buf, size_t size);
int  sys_mkdir(const char *path, int mode);

struct fb_info; /* defined in <abi/syscall.h> */
int          sys_fb_info(struct fb_info *out);
int          sys_fb_blit(const void *xrgb, unsigned w, unsigned h);
int          sys_key_get(void);
unsigned long sys_uptime_ms(void);

/* Process control. sys_spawn loads path as a new user task (argv NULL-terminated,
 * argv[0] conventionally the program name) and returns its pid without waiting.
 * sys_waitpid blocks until a child exits: pid > 0 for that child, pid <= 0 for
 * any; *status gets the raw exit code; returns the child's pid or -ECHILD. */
int sys_spawn(const char *path, char *const argv[]);
int sys_waitpid(int pid, int *status, int options);
int sys_yield(void);

__attribute__((noreturn)) void _exit(int code);
