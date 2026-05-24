#include <early_alloc.h>
#include <framebuffer.h>
#include <gdt.h>
#include <idt.h>
#include <init.h>
#include <isr_vector.h>
#include <kernelcfg.h>
#include <keyboard.h>
#include <kmalloc.h>
#include <panic.h>
#include <pic.h>
#include <stddef.h>
#include <stdio.h>
#include <syscall.h>
#include <task.h>
#include <tty.h>
#include <types.h>

extern u8 kernel_stack[];

static struct idt_entry idt[IDT_ENTRIES] __page_aligned;
static struct idt_ptr idt_descriptor;

void (**isr_table)(struct cpu_state *regs);

#include <sched.h>

extern u8 scheduler_enabled;

void isr_common_entry(u64 int_id, struct cpu_state *regs)
{
    ptrace = regs;

    if (int_id < 32)
    {
        /* Exception handler */
        if (scheduler_enabled && current_task && current_task != &idle)
        {
            /* Redirect return pointer to task_exit */
            regs->rip = (u64)task_exit;
            return;
        }
        else
        {
            kpanic("EXCEPTION %d during KERNEL BOOT at RIP: %p", (int)int_id, (void *)regs->rip);
        }
    }

    if (isr_table && isr_table[int_id])
    {
        isr_table[int_id](regs);
        PIC_sendEOI(15);
        return;
    }
}

void set_idt_gate(int vector, void (*isr)(), u8 flags)
{
    u64 addr = (u64)isr;

    idt[vector].offset_low = addr & 0xFFFF;
    idt[vector].selector = KERNEL_CS;
    idt[vector].ist = 0;
    idt[vector].type_attr = flags;
    idt[vector].offset_mid = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero = 0;
}

static inline void idt_load()
{
    asm volatile("lidt %0\n" : : "m"(idt_descriptor) : "memory");
}

int init_idt()
{
    isr_table = zalloc(sizeof(void *) * IDT_ENTRIES);

    for (int i = 0; i < IDT_ENTRIES; i++)
        set_idt_gate(i, isr_stub_table[i], 0x8E);

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (u64)&idt;

    idt_load();

    return 0;
}

core_initcall(init_idt);
