#include <abi/errno.h>
#include <lizard/boot.h>
#include <lizard/idt.h>
#include <lizard/init.h>
#include <lizard/isr_vector.h>
#include <lizard/syscall.h>
#include <lizard/tty.h>
#include <nolibc/stdio.h>
#include <nolibc/types.h>

#define SYSCALL_ISR_INDEX 0x80

static syscall_fn syscall_table[SYS_NR_MAX];

/* ---- individual syscalls ------------------------------------------------- */

static int user_ptr_ok(long p, long len)
{
    if (p == 0 || len < 0) return 0;
    /* reject the kernel half; the user PML4 is the live CR3 here so anything
     * below the HHDM base is a genuine user mapping (or it faults). */
    if ((u64)p >= BOOT_HHDM_BASE) return 0;
    if ((u64)p + (u64)len < (u64)p) return 0; /* wrap */
    return 1;
}

static long sys_write(long fd, long ubuf, long len, long a3, long a4, long a5)
{
    (void)a3; (void)a4; (void)a5;
    if (fd != 1 && fd != 2) return -EBADF;
    if (!user_ptr_ok(ubuf, len)) return -EFAULT;

    tty_write((const char *)ubuf, (size_t)len);
    return len;
}

static long sys_read(long fd, long ubuf, long len, long a3, long a4, long a5)
{
    (void)fd; (void)ubuf; (void)len; (void)a3; (void)a4; (void)a5;
    return -ENOSYS; /* no console input yet */
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
    if (current_task) current_task->exit_code = (int)code;
    task_exit(); /* never returns */
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
    syscall_table[SYS_exit]   = sys_exit;
    syscall_table[SYS_write]  = sys_write;
    syscall_table[SYS_read]   = sys_read;
    syscall_table[SYS_sleep]  = sys_sleep;
    syscall_table[SYS_getpid] = sys_getpid;

    /* DPL 3 interrupt gate to a dedicated stub that preserves RDI (arg1). */
    set_idt_gate(SYSCALL_ISR_INDEX, isr_syscall_stub, 0xEE);

    return 0;
}

device_initcall(syscall_init);
