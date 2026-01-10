#include <syscall.h>
#include <stdio.h>

static syscall_handler syscall_table[MAX_SYSCALLS];

void syscall_init()
{
    // For now, we only have one syscall
    syscall_table[0] = &sys_sleep;
}

void sys_sleep(CpuState *regs)
{
    u32 ms = regs->rbx;
    kprintf("sys_sleep called with %u ms\n", ms);
    task_sleep(ms);
}

void syscall_handler_c(CpuState *regs)
{
    u32 syscall_num = regs->rax;
    if (syscall_num < MAX_SYSCALLS && syscall_table[syscall_num] != NULL)
    {
        syscall_handler handler = syscall_table[syscall_num];
        handler(regs);
    }
    else
    {
        kprintf("Unknown syscall %u\n", syscall_num);
    }
}

u64 syscall0(u64 syscall_num)
{
    u64 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(syscall_num));
    return ret;
}

u64 syscall1(u64 syscall_num, u64 arg1)
{
    u64 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(syscall_num), "b"(arg1));
    return ret;
}

u64 syscall2(u64 syscall_num, u64 arg1, u64 arg2)
{
    u64 ret;
    asm volatile("int $0x80" : "=a"(ret) : "a"(syscall_num), "b"(arg1), "c"(arg2));
    return ret;
}

