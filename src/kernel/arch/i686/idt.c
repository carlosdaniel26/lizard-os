#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <kernel/utils/alias.h>
#include <kernel/arch/idt.h>
#include <kernel/terminal/terminal.h>
#include <kernel/terminal/vga.h>
#include <kernel/drivers/keyboard.h>
#include <kernel/drivers/rtc.h>
#include <kernel/utils/io.h>
#include <kernel/cpu/isr.h>
#include <kernel/cpu/pic.h>

#define IDT_ENTRIES 256

idt_entry_struct idt[IDT_ENTRIES];
idt_ptr_struct ptr_idt;

bool vectors[IDT_ENTRIES];

void set_idt_descriptor(uint8_t vector, void (*isr)(), uint8_t flags)
{
    idt_entry_struct* descriptor = &idt[vector];

    descriptor->base_low  = (uint32_t)isr & 0xFFFF; // Get just the first 16 bits
    descriptor->selector  = 0x08;
    descriptor->flags     = flags;
    descriptor->base_high = ((uint32_t)isr >> 16) & 0xFFFF;
    descriptor->always0   = 0;
}

void init_idt(void)
{
    PIC_remap();
    
    ptr_idt.base  = (uint32_t)&idt[0];
    ptr_idt.limit = sizeof(idt_entry_struct) * IDT_ENTRIES - 1;


    // Set IDT descriptors
    set_idt_descriptor(0,  isr_stub_divide_by_zero,  0x8E);
    set_idt_descriptor(6,  isr_stub_invalid_opcode,  0x8E);
    set_idt_descriptor(14, isr_stub_page_fault, 0x8E);
    //set_idt_descriptor(32, stub_32, 0x8E);  //timer
    set_idt_descriptor(40, isr_timer, 0x8E);  // RTC timer
    set_idt_descriptor(33, isr_keyboard, 0x8E);  // keyboard



    __asm__("lidt %0" : : "m"(ptr_idt));
}