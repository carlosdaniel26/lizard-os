#include <syscall.h>

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
