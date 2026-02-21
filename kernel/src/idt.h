#pragma once

#include <task.h>
#include <types.h>

#define IDT_ENTRIES 256

typedef struct {
    u16 offset_low;  /* -> 0-15 */
    u16 selector;    /* -> 0-15 */
    u8 ist;          /* -> 0-7 */
    u8 type_attr;    /* -> 0-7 */
    u16 offset_mid;  /* -> 16-31 */
    u32 offset_high; /* -> 32-63 */
    u32 zero;        /* -> 0-31 */
} __attribute__((packed)) idt_entry;

typedef struct {
    u16 limit;
    u64 base;
} __attribute__((packed)) idt_ptr;

void isr_common_entry(u64 int_id, CpuState *regs);
void init_idt();
void set_idt_gate(int vector, void (*isr)(), u8 flags);
