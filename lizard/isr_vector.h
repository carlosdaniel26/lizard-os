#pragma once

#include <lizard/idt.h>

extern void *isr_stub_table[IDT_ENTRIES];

/* Dedicated int 0x80 entry (lizard/isr_vector.asm) - preserves RDI. */
void isr_syscall_stub(void);
