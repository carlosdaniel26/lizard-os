#include <stdio.h>
#include <sys/syscall.h>

int main(void)
{
    puts("hello from userspace");
    puts("goodbye");
    return 7;
}
