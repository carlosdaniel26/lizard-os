#define stack_init(stack, size) \
    asm volatile("mov %0, %%rsp" : : "r"(&kernel_stack[KERNEL_STACK_SIZE]) : "memory")