#include <exception.h>
#include <framebuffer.h>
#include <init.h>
#include <isr_vector.h>
#include <kernelcfg.h>

#define EXCEPTION_PAGE_FAULT 14

void exception_pagefault(struct cpu_state *regs)
{
    u64 faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

    extern u8 kernel_stack[];
    extern u32 kernel_start;
    extern u32 kernel_end;

    if (faulting_address >= (u64)&kernel_start && faulting_address < (u64)&kernel_end)
    {
        kpanic("Page Fault in kernel space at address 0x%x, RIP: 0x%x, Error Code: 0x%x", faulting_address,
               regs->rip, regs->rflags);
    }

    if (((u64)framebuffer <= faulting_address && faulting_address < ((u64)framebuffer + framebuffer_length)))
    {
        kpanic("Page Fault in framebuffer space at address 0x%x, RIP: 0x%x, Error Code: 0x%x",
               faulting_address, regs->rip, regs->rflags);
    }

    /* possible stack fault? */
    if (regs->rsp < (uintptr_t)&kernel_stack && regs->rsp >= (uintptr_t)&kernel_stack[KERNEL_STACK_SIZE])
    {
        kpanic("Page Fault in kernel stack at address 0x%x, RIP: 0x%x, Error Code: 0x%x", faulting_address,
               regs->rip, regs->rflags);
    }

    kpanic("Unknown Page Fault at address 0x%x, RIP: 0x%x, Error Code: 0x%x", faulting_address, regs->rip,
           regs->rflags);
}

int init_exception()
{
    isr_table[EXCEPTION_PAGE_FAULT] = &exception_pagefault;

    return 0;
}

postcore_initcall(init_exception);