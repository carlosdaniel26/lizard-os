#include <early_alloc.h>
#include <framebuffer.h>
#include <idt.h>
#include <isr_vector.h>
#include <kernelcfg.h>
#include <keyboard.h>
#include <panic.h>
#include <pic.h>
#include <stdio.h>
#include <syscall.h>
#include <task.h>
#include <tty.h>
#include <types.h>

#define SCHEDULER_ISR_INDEX 48
#define SYSCALL_ISR_INDEX 0x80

extern u8 kernel_stack[];

static idt_entry idt[IDT_ENTRIES];
static idt_ptr idt_descriptor;

void (*isr_table[IDT_ENTRIES])(CpuState *regs);

#define EXCEPTION_PAGE_FAULT 14

void isr_scheduler(CpuState *regs)
{
    scheduler(regs);
}

void isr_syscall(CpuState *regs)
{
    syscall_handler_c(regs);
}

void isr_common_entry(u64 int_id, CpuState *regs)
{
    PIC_sendEOI(15);

    if (isr_table[int_id])
    {
        isr_table[int_id](regs);
        return;
    }

    if (EXCEPTION_PAGE_FAULT == int_id)
    {
        u64 faulting_address;
        asm volatile("mov %%cr2, %0" : "=r"(faulting_address));

        extern u32 kernel_start;
        extern u32 kernel_end;

        if (faulting_address >= (u64)&kernel_start && faulting_address < (u64)&kernel_end)
        {
            kpanic("Page Fault in kernel space at address 0x%x, RIP: 0x%x, Error Code: 0x%x",
                   faulting_address, regs->rip, regs->rflags);
        }

        if ((hhdm_offset + early_base) <= faulting_address && faulting_address < (hhdm_offset + early_end))
        {
            kpanic("Page Fault in early alloc space at address 0x%x, RIP: 0x%x, Error Code: 0x%x",
                   faulting_address, regs->rip, regs->rflags);
        }

        if (((u64)framebuffer <= faulting_address &&
             faulting_address < ((u64)framebuffer + framebuffer_length)))
        {
            kpanic("Page Fault in framebuffer space at address 0x%x, RIP: 0x%x, Error Code: 0x%x",
                   faulting_address, regs->rip, regs->rflags);
        }

        /* possible stack fault? */
        if (regs->rsp < (uintptr_t)&kernel_stack && regs->rsp >= (uintptr_t)&kernel_stack[KERNEL_STACK_SIZE])
        {
            kpanic("Page Fault in kernel stack at address 0x%x, RIP: 0x%x, Error Code: 0x%x",
                   faulting_address, regs->rip, regs->rflags);
        }

        kpanic("Unknown Page Fault at address 0x%x, RIP: 0x%x, Error Code: 0x%x", faulting_address, regs->rip,
               regs->rflags);
    }
}

void set_idt_gate(int vector, void (*isr)(void), u8 flags)
{
    u64 addr = (u64)isr;

    idt[vector].offset_low = addr & 0xFFFF;
    idt[vector].selector = 0x08;
    idt[vector].ist = 0;
    idt[vector].type_attr = flags;
    idt[vector].offset_mid = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero = 0;
}

static inline void idt_load(void)
{
    asm volatile("lidt %0\n" : : "m"(idt_descriptor) : "memory");
}

void init_idt(void)
{
    for (int i = 0; i < IDT_ENTRIES; i++)
        set_idt_gate(i, isr_vectors[i], 0x8E);

    set_idt_gate(SYSCALL_ISR_INDEX, isr_vectors[SYSCALL_ISR_INDEX], 0xEE);

    isr_table[SCHEDULER_ISR_INDEX] = &isr_scheduler;
    isr_table[SYSCALL_ISR_INDEX] = &isr_syscall;

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (u64)&idt;

    idt_load(void);
}
