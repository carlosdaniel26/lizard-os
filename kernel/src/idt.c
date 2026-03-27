#include <early_alloc.h>
#include <framebuffer.h>
#include <idt.h>
#include <init.h>
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

extern u8 kernel_stack[];

static idt_entry idt[IDT_ENTRIES];
static idt_ptr idt_descriptor;

void (*isr_table[IDT_ENTRIES])(CpuState *regs);

void isr_common_entry(u64 int_id, CpuState *regs)
{
    ptrace = regs;

    if (isr_table[int_id])
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
    idt[vector].selector = 0x08;
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
    for (int i = 0; i < IDT_ENTRIES; i++)
        set_idt_gate(i, isr_vectors[i], 0x8E);

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (u64)&idt;

    idt_load();

    return 0;
}

core_initcall(init_idt);
