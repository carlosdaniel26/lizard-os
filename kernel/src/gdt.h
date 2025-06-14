#include <stdint.h>

typedef struct global_descriptor {
    uint16_t limit_low;       // bits 0-15 do limit
    uint16_t base_low;        // bits 0-15 do base
    uint8_t  base_middle;     // bits 16-23 do base
    uint8_t  access;          // byte de acesso
    uint8_t  granularity;     // bits 16-19 do limit + flags
    uint8_t  base_high;       // bits 24-31 do base
} __attribute__((packed)) global_descriptor;

typedef struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_ptr;

global_descriptor create_gdt_gate(uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity);
void init_gdt();