#include <idt.h>
#include <isr_vector.h>
#include <keyboard.h>
#include <pic.h>
#include <stdint.h>
#include <stdio.h>
#include <task.h>

static idt_entry idt[IDT_ENTRIES];
static idt_ptr idt_descriptor;

void (*isr_table[IDT_ENTRIES])(CpuState *regs);

void isr_common_entry(uint64_t int_id, CpuState *regs)
{
	PIC_sendEOI(15);

	if (isr_table[int_id])
	{
		isr_table[int_id](regs);
	}
}

void set_idt_gate(int vector, void (*isr)(), uint8_t flags)
{
	uint64_t addr = (uint64_t)isr;

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
	asm volatile("lidt %0\n"
				 "sti\n"
				 :
				 : "m"(idt_descriptor)
				 : "memory");
}

void init_idt()
{
	for (int i = 0; i < IDT_ENTRIES; i++)
		set_idt_gate(i, isr_vectors[i], 0x8E);

	idt_descriptor.limit = sizeof(idt) - 1;
	idt_descriptor.base = (uint64_t)&idt;

	idt_load();
}
