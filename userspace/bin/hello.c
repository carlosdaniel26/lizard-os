#include <stdio.h>
#include <sys/syscall.h>

int main(void)
{
    printf("hello from userspace, pid=%d\n", sys_getpid());
    puts("goodbye");
    return 0;
}
