#include <lizard/early_alloc.h>
#include <lizard/exception.h>
#include <lizard/framebuffer.h>
#include <lizard/gdt.h>
#include <lizard/idt.h>
#include <lizard/init.h>
#include <lizard/isr_vector.h>
#include <lizard/kernelcfg.h>
#include <lizard/keyboard.h>
#include <lizard/kmalloc.h>
#include <lizard/panic.h>
#include <lizard/pic.h>
#include <nolibc/stddef.h>
#include <nolibc/stdio.h>
#include <lizard/syscall.h>
#include <lizard/task.h>
#include <lizard/tty.h>
#include <nolibc/types.h>

extern u8 kernel_stack[];

static struct idt_entry idt[IDT_ENTRIES] __page_aligned;
static struct idt_ptr idt_descriptor;

void (*isr_table[IDT_ENTRIES])(struct cpu_state *regs);

#include <lizard/sched.h>

extern u8 scheduler_enabled;

void isr_common_entry(u64 int_id, struct cpu_state *regs)
{
    ptrace = regs;

    if (int_id < 32)
    {
        exception_handle(regs);
        return;
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
    memset(isr_table, 0, sizeof(isr_table));

    /* isr_stub_table[i] is the absolute address of isr_vector_<i> (dq in
     * isr_vector.asm), so use it directly as the gate handler. */
    for (int i = 0; i < IDT_ENTRIES; i++)
        set_idt_gate(i, (void *)isr_stub_table[i], 0x8E);

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base = (u64)&idt;

    idt_load();

    return 0;
}

core_initcall(init_idt);
