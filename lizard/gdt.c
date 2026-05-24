#include <lizard/gdt.h>
#include <lizard/init.h>
#include <lizard/memory_map.h>
#include <lizard/kmalloc.h>
#include <nolibc/string.h>
#include <lizard/panic.h>
#include <nolibc/stdbool.h>
#include <nolibc/stdio.h>
#include <lizard/tss.h>

static struct global_descriptor boot_gdt[5] __initdata;
static struct gdt_ptr boot_gdt_ptr __initdata;

static struct global_descriptor runtime_gdt[GDT_MAX_ENTRIES];
static struct gdt_ptr runtime_gdt_ptr;
static int gdt_next_index = 0;
static bool gdt_is_dynamic = false;

struct global_descriptor gdt_create_gate(u64 base, u64 limit, u8 access, u8 granularity)
{
    struct global_descriptor gate;

    gate.limit_low = (limit & 0xFFFF);
    gate.base_low = (base & 0xFFFF);
    gate.base_middle = (base >> 16) & 0xFF;
    gate.access = access;
    gate.granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    gate.base_high = (base >> 24) & 0xFF;

    return gate;
}

int gdt_add_gate(u64 base, u64 limit, u8 access, u8 granularity)
{
    if (!gdt_is_dynamic)
        kpanic("GDT: Attempted to add gate before dynamic initialization");

    if (gdt_next_index >= GDT_MAX_ENTRIES)
        kpanic("GDT: Maximum entries (%d) exceeded", GDT_MAX_ENTRIES);

    runtime_gdt[gdt_next_index] = gdt_create_gate(base, limit, access, granularity);
    return gdt_next_index++;
}

int gdt_add_tss_gate(u64 base, u64 limit, u8 access, u8 granularity)
{
    if (!gdt_is_dynamic)
        kpanic("GDT: Attempted to add gate before dynamic initialization");

    if (gdt_next_index + 1 >= GDT_MAX_ENTRIES)
        kpanic("GDT: Maximum entries exceeded");

    runtime_gdt[gdt_next_index] = gdt_create_gate(base, limit, access, granularity);

    u64 *second_half = (u64 *)&runtime_gdt[gdt_next_index + 1];
    *second_half = (base >> 32);

    int index = gdt_next_index;
    gdt_next_index += 2;
    return index;
}

void gdt_load(struct gdt_ptr *ptr)
{
    asm volatile("lgdt %0\n"

                 /* Far jump to reload CS */
                 "pushq $0x08\n"
                 "leaq 1f(%%rip), %%rax\n"
                 "pushq %%rax\n"
                 "lretq\n"

                 /* Continue execution with new CS */
                 "1:\n"

                 /* Load new data segment selector */
                 "mov $0x10, %%ax\n"
                 "mov %%ax, %%ds\n"
                 "mov %%ax, %%es\n"
                 "mov %%ax, %%fs\n"
                 "mov %%ax, %%gs\n"
                 "mov %%ax, %%ss\n"
                 :
                 : "m"(*ptr)
                 : "memory", "rax", "ax");
}

void __init gdt_init_dynamic()
{
    memset(runtime_gdt, 0, sizeof(struct global_descriptor) * GDT_MAX_ENTRIES);

    /* Copy boot GDT entries */
    for (int i = 0; i < 5; i++) {
        runtime_gdt[i] = boot_gdt[i];
    }
    gdt_next_index = 5;
    gdt_is_dynamic = true;

    runtime_gdt_ptr.limit = (sizeof(struct global_descriptor) * GDT_MAX_ENTRIES) - 1;
    runtime_gdt_ptr.base = (u64)runtime_gdt;

    gdt_load(&runtime_gdt_ptr);

    tss_init();

    kprintf("GDT: Dynamic GDT initialized with %d entries\n", GDT_MAX_ENTRIES);
}

static int __init init_gdt()
{
    boot_gdt[0] = gdt_create_gate(0, 0, 0x00, 0x00); // Null
    boot_gdt[1] = gdt_create_gate(0, 0xFFFFFFFF, GDT_KERNEL_CODE, GDT_GRAN_4KB | GDT_GRAN_64BIT | GDT_GRAN_LIMIT_HIGH);
    boot_gdt[2] = gdt_create_gate(0, 0xFFFFFFFF, GDT_KERNEL_DATA, GDT_GRAN_4KB | GDT_GRAN_64BIT | GDT_GRAN_LIMIT_HIGH);
    boot_gdt[3] = gdt_create_gate(0, 0xFFFFFFFF, GDT_USER_CODE,   GDT_GRAN_4KB | GDT_GRAN_64BIT | GDT_GRAN_LIMIT_HIGH);
    boot_gdt[4] = gdt_create_gate(0, 0xFFFFFFFF, GDT_USER_DATA,   GDT_GRAN_4KB | GDT_GRAN_64BIT | GDT_GRAN_LIMIT_HIGH);

    boot_gdt_ptr.limit = (sizeof(struct global_descriptor) * 5) - 1;
    boot_gdt_ptr.base = (u64)&boot_gdt;

    gdt_load(&boot_gdt_ptr);

    return 0;
}

core_initcall(init_gdt);
