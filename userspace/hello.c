#include "../lizard/syscall.h"

int main()
{
    syscall1(1, (u64)"Hello, World!\n");
    return 0;
}
