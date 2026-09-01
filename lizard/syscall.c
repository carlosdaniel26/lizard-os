#include <abi/errno.h>
#include <lizard/boot.h>
#include <lizard/framebuffer.h>
#include <lizard/idt.h>
#include <lizard/init.h>
#include <lizard/isr_vector.h>
#include <lizard/keyboard.h>
#include <lizard/ktime.h>
#include <lizard/syscall.h>
#include <lizard/tty.h>
#include <lizard/vfs.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <nolibc/types.h>

#define SYSCALL_ISR_INDEX 0x80

static syscall_fn syscall_table[SYS_NR_MAX];

/* ---- user-pointer validation ------------------------------------------- */

static int user_ptr_ok(long p, long len)
{
    if (p == 0 || len < 0) return 0;
    /* reject the kernel half; the user PML4 is the live CR3 here so anything
     * below the HHDM base is a genuine user mapping (or it faults). */
    if ((u64)p >= BOOT_HHDM_BASE) return 0;
    if ((u64)p + (u64)len < (u64)p) return 0; /* wrap */
    return 1;
}

/* Copy a NUL-terminated string in from user space. Returns 0 on success,
 * -errno otherwise. `max` includes the terminator. */
static long copy_user_str(long uptr, char *dst, size_t max)
{
    if (!user_ptr_ok(uptr, (long)max)) return -EFAULT;

    const char *s = (const char *)uptr;
    for (size_t i = 0; i < max; i++)
    {
        dst[i] = s[i];
        if (s[i] == '\0') return 0;
    }
    return -EINVAL; /* not terminated within max */
}

/* ---- per-task fd table ------------------------------------------------- */

static int fd_alloc(struct file *f)
{
    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (!current_task->fd_table[i])
        {
            current_task->fd_table[i] = f;
            return TASK_FD_BASE + i;
        }
    }
    return -1;
}

static struct file *fd_get(long fd)
{
    if (!current_task) return NULL;
    if (fd < TASK_FD_BASE || fd >= TASK_FD_BASE + TASK_MAX_FDS) return NULL;
    return current_task->fd_table[fd - TASK_FD_BASE];
}

static void task_close_all_fds(struct task *t)
{
    for (int i = 0; i < TASK_MAX_FDS; i++)
    {
        if (t->fd_table[i])
        {
            vfs_close(t->fd_table[i]);
            t->fd_table[i] = NULL;
        }
    }
}

/* ---- individual syscalls --------------------------------------------- */

static long sys_write(long fd, long ubuf, long len, long a3, long a4, long a5)
{
    (void)a3; (void)a4; (void)a5;
    if (!user_ptr_ok(ubuf, len)) return -EFAULT;

    if (fd == 1 || fd == 2)
    {
        tty_write((const char *)ubuf, (size_t)len);
        return len;
    }

    struct file *f = fd_get(fd);
    if (!f) return -EBADF;
    return vfs_write(f, (const void *)ubuf, (size_t)len);
}

static long sys_read(long fd, long ubuf, long len, long a3, long a4, long a5)
{
    (void)a3; (void)a4; (void)a5;
    if (!user_ptr_ok(ubuf, len)) return -EFAULT;
    if (fd == 0 || fd == 1 || fd == 2) return -ENOSYS; /* no console input yet */

    struct file *f = fd_get(fd);
    if (!f) return -EBADF;
    return vfs_read(f, (void *)ubuf, (size_t)len);
}

static long sys_open(long upath, long flags, long a2, long a3, long a4, long a5)
{
    (void)a2; (void)a3; (void)a4; (void)a5;

    char path[128];
    char abspath[130];
    long err = copy_user_str(upath, path, sizeof(path));
    if (err) return err;

    const char *p = path;
    if (p[0] != '/')
    {
        abspath[0] = '/';
        size_t n = strlen(path);
        if (n > sizeof(abspath) - 2) n = sizeof(abspath) - 2;
        memcpy(abspath + 1, path, n);
        abspath[n + 1] = '\0';
        p = abspath;
    }

    struct file *f = vfs_open(p, (int)flags);
    if (!f) return -ENOENT;

    int fd = fd_alloc(f);
    if (fd < 0)
    {
        vfs_close(f);
        return -ENOMEM;
    }
    return fd;
}

static long sys_close(long fd, long a1, long a2, long a3, long a4, long a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    struct file *f = fd_get(fd);
    if (!f) return -EBADF;

    vfs_close(f);
    current_task->fd_table[fd - TASK_FD_BASE] = NULL;
    return 0;
}

static long sys_lseek(long fd, long off, long whence, long a3, long a4, long a5)
{
    (void)a3; (void)a4; (void)a5;
    struct file *f = fd_get(fd);
    if (!f) return -EBADF;

    long base;
    switch (whence)
    {
    case LZ_SEEK_SET: base = 0; break;
    case LZ_SEEK_CUR: base = (long)f->offset; break;
    case LZ_SEEK_END: base = f->inode ? (long)f->inode->size : 0; break;
    default: return -EINVAL;
    }

    long pos = base + off;
    if (pos < 0) return -EINVAL;

    f->offset = (u64)pos;
    return pos;
}

static long sys_fb_info(long uout, long a1, long a2, long a3, long a4, long a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (!user_ptr_ok(uout, (long)sizeof(struct fb_info))) return -EFAULT;

    struct fb_info *out = (struct fb_info *)uout;
    out->width = (u32)width;
    out->height = (u32)height;
    out->pitch = pitch;
    out->bpp = 32;
    return 0;
}

static long sys_fb_blit(long usrc, long w, long h, long a3, long a4, long a5)
{
    (void)a3; (void)a4; (void)a5;
    if (w <= 0 || h <= 0 || w > 8192 || h > 8192) return -EINVAL;
    if (!user_ptr_ok(usrc, w * h * 4)) return -EFAULT;

    fb_present_xrgb((const u32 *)usrc, (u32)w, (u32)h);
    return 0;
}

static long sys_key_get(long a0, long a1, long a2, long a3, long a4, long a5)
{
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    keyboard_set_raw(1); /* first read hands the keyboard to the caller */
    int sc = keyboard_pop_scancode();
    return sc < 0 ? 0 : sc;
}

static long sys_uptime_ms(long a0, long a1, long a2, long a3, long a4, long a5)
{
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return (long)time_uptime_ms();
}

static long sys_sleep(long ms, long a1, long a2, long a3, long a4, long a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (ms < 0) return -EINVAL;
    task_sleep((u32)ms);
    return 0;
}

static long sys_getpid(long a0, long a1, long a2, long a3, long a4, long a5)
{
    (void)a0; (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    return current_task ? (long)current_task->pid : 0;
}

static long sys_exit(long code, long a1, long a2, long a3, long a4, long a5)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5;
    if (current_task)
    {
        current_task->exit_code = (int)code;
        task_close_all_fds(current_task);
    }
    keyboard_set_raw(0); /* give the keyboard back to the shell */
    task_exit();         /* never returns */
    __builtin_unreachable();
}

/* ---- dispatch ---------------------------------------------------------- */

void syscall_handler_c(struct cpu_state *regs)
{
    u64 nr = regs->rax;
    long ret;

    if (nr < SYS_NR_MAX && syscall_table[nr])
        ret = syscall_table[nr](regs->rdi, regs->rsi, regs->rdx,
                                regs->r10, regs->r8, regs->r9);
    else
        ret = -ENOSYS;

    regs->rax = (u64)ret; /* isr_syscall_stub pops this into RAX before iretq */
}

static int syscall_init(void)
{
    syscall_table[SYS_exit]      = sys_exit;
    syscall_table[SYS_write]     = sys_write;
    syscall_table[SYS_read]      = sys_read;
    syscall_table[SYS_sleep]     = sys_sleep;
    syscall_table[SYS_getpid]    = sys_getpid;
    syscall_table[SYS_open]      = sys_open;
    syscall_table[SYS_close]     = sys_close;
    syscall_table[SYS_lseek]     = sys_lseek;
    syscall_table[SYS_fb_info]   = sys_fb_info;
    syscall_table[SYS_fb_blit]   = sys_fb_blit;
    syscall_table[SYS_key_get]   = sys_key_get;
    syscall_table[SYS_uptime_ms] = sys_uptime_ms;

    /* DPL 3 interrupt gate to a dedicated stub that preserves RDI (arg1). */
    set_idt_gate(SYSCALL_ISR_INDEX, isr_syscall_stub, 0xEE);

    return 0;
}

device_initcall(syscall_init);
