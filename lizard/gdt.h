#pragma once

#include <lizard/init.h>
#include <nolibc/types.h>

#define GDT_MAX_ENTRIES 1024

struct global_descriptor {
    u16 limit_low;  /* -> 0-15 */
    u16 base_low;   /* -> 0-15 */
    u8 base_middle; /* -> 16-23 */
    u8 access;      /* -> 17 */
    u8 granularity; /* -> 16-19 */
    u8 base_high;   /* -> 24-31 */
} __attribute__((packed));

struct gdt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));

/* Access Flags */
#define GDT_ACCESS_PRESENT     (1 << 7)
#define GDT_ACCESS_DPL0        (0 << 5)
#define GDT_ACCESS_DPL3        (3 << 5)
#define GDT_ACCESS_S           (1 << 4)
#define GDT_ACCESS_CODE        (1 << 3)
#define GDT_ACCESS_DATA        (0 << 3)
#define GDT_ACCESS_EX          (1 << 3)
#define GDT_ACCESS_RW          (1 << 1)

#define GDT_KERNEL_CODE (GDT_ACCESS_PRESENT | GDT_ACCESS_S | GDT_ACCESS_EX | GDT_ACCESS_RW)
#define GDT_KERNEL_DATA (GDT_ACCESS_PRESENT | GDT_ACCESS_S | GDT_ACCESS_RW)
#define GDT_USER_CODE   (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 | GDT_ACCESS_S | GDT_ACCESS_EX | GDT_ACCESS_RW)
#define GDT_USER_DATA   (GDT_ACCESS_PRESENT | GDT_ACCESS_DPL3 | GDT_ACCESS_S | GDT_ACCESS_RW)

#define KERNEL_CS 0x08
#define KERNEL_SS 0x10
#define USER_CS   0x1B
#define USER_SS   0x23
#define TSS_SEL   0x28

/* Granularity Flags */
#define GDT_GRAN_4KB           (1 << 7)
#define GDT_GRAN_64BIT         (1 << 5)
#define GDT_GRAN_LIMIT_HIGH    0x0F

struct global_descriptor create_gdt_gate(u64 base, u64 limit, u8 access, u8 granularity);
int gdt_add_gate(u64 base, u64 limit, u8 access, u8 granularity);
int gdt_add_tss_gate(u64 base, u64 limit, u8 access, u8 granularity);
void gdt_load(struct gdt_ptr *ptr);
void gdt_init_dynamic();

