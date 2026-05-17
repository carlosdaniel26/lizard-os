#include <gdt.h>
#include <init.h>
#include <memory_map.h>

static struct global_descriptor boot_gdt[5] __initdata;
static struct gdt_ptr boot_gdt_ptr __initdata;

struct global_descriptor __init create_gdt_gate(u64 base, u64 limit, u8 access, u8 granularity)
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

static inline void __init gdt_load()
{
    boot_gdt_ptr.limit = (sizeof(struct global_descriptor) * 5) - 1;
    boot_gdt_ptr.base = (u64)&boot_gdt;

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
                 : "m"(boot_gdt_ptr)
                 : "memory", "rax", "ax");
}

static int __init init_gdt()
{
    /*
     * Access bytes:
     * 0x9A = 10011010 (Ring 0, Code, Exec/Read)
     * 0x92 = 10010010 (Ring 0, Data, Read/Write)
     * 0xFA = 11111010 (Ring 3, Code, Exec/Read)
     * 0xF2 = 11110010 (Ring 3, Data, Read/Write)
     */
    boot_gdt[0] = create_gdt_gate(0, 0, 0x00, 0x00); // Null
    boot_gdt[1] = create_gdt_gate(0, 0xFFFFFFFF, 0x9A, 0xA0); // Kernel Code
    boot_gdt[2] = create_gdt_gate(0, 0xFFFFFFFF, 0x92, 0xA0); // Kernel Data
    boot_gdt[3] = create_gdt_gate(0, 0xFFFFFFFF, 0xFA, 0xA0); // User Code
    boot_gdt[4] = create_gdt_gate(0, 0xFFFFFFFF, 0xF2, 0xA0); // User Data

    gdt_load();

    return 0;
}

core_initcall(init_gdt);
