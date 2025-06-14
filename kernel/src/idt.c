#include <stdint.h>
#include <stdio.h>
#include <idt.h>
#include <pic.h>

static idt_entry idt[IDT_ENTRIES];
static idt_ptr idt_descriptor;

static void set_idt_gate(int vector, void (*isr)(), uint8_t flags)
{
    uint64_t addr = (uint64_t)isr;

    idt[vector].offset_low  = addr & 0xFFFF;
    idt[vector].selector    = 0x08;
    idt[vector].ist         = 0;
    idt[vector].type_attr   = flags;
    idt[vector].offset_mid  = (addr >> 16) & 0xFFFF;
    idt[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    idt[vector].zero        = 0;
}

static inline void idt_load()
{
    asm volatile (
        "lidt %0\n"
        "sti\n"
        :
        : "m"(idt_descriptor)
        : "memory"
    );
}

__attribute__((interrupt))
void default_handler(void* frame)
{
    pic_send_eoi(15);
    return;
    (void)frame;
    while (1) __asm__("hlt");
}

void init_idt()
{
    for (int i = 0; i < IDT_ENTRIES; i++)
        set_idt_gate(i, default_handler, 0x8E);

    idt_descriptor.limit = sizeof(idt) - 1;
    idt_descriptor.base  = (uint64_t)&idt;

    idt_load();
}
