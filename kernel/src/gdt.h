#pragma once

#include <types.h>

typedef struct global_descriptor {
    u16 limit_low;  /* -> 0-15 */
    u16 base_low;   /* -> 0-15 */
    u8 base_middle; /* -> 16-23 */
    u8 access;      /* -> 17 */
    u8 granularity; /* -> 16-19 */
    u8 base_high;   /* -> 24-31 */
} __attribute__((packed)) global_descriptor;

typedef struct gdt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed)) gdt_ptr;

global_descriptor create_gdt_gate(u64 base, u64 limit, u8 access, u8 granularity);
void init_gdt();