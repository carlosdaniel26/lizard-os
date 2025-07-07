#ifndef IDT_H
#define IDT_H

#include <stdint.h>
#include <task.h>

#define IDT_ENTRIES 256

typedef struct {
    uint16_t offset_low;     /* -> 0-15 */
    uint16_t selector;       /* -> 0-15 */
    uint8_t  ist;            /* -> 0-7 */
    uint8_t  type_attr;      /* -> 0-7 */
    uint16_t offset_mid;     /* -> 16-31 */
    uint32_t offset_high;    /* -> 32-63 */
    uint32_t zero;           /* -> 0-31 */
} __attribute__((packed)) idt_entry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr;

void isr_common_entry(uint64_t int_id, CpuState *regs);
void init_idt();
void set_idt_gate(int vector, void (*isr)(), uint8_t flags);

#endif