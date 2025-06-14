#include <gdt.h>

global_descriptor gdt[5];
gdt_ptr gdt_pointer;

global_descriptor create_gdt_gate(uint64_t base, uint64_t limit, uint8_t access, uint8_t granularity) 
{
    global_descriptor gate;

    gate.limit_low = (limit & 0xFFFF);
    gate.base_low = (base & 0xFFFF);
    gate.base_middle = (base >> 16) & 0xFF;
    gate.access = access;
    gate.granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    gate.base_high = (base >> 24) & 0xFF;

    return gate;
}

static inline void gdt_load()
{
    gdt_pointer.limit = (sizeof(global_descriptor) * 5) - 1;
    gdt_pointer.base = (uint64_t)&gdt;

    asm volatile (
	    "lgdt %0\n"

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
	    : "m"(gdt_pointer)
	    : "memory", "rax", "ax"
	);
}

void init_gdt()
{
	gdt[0] = create_gdt_gate(0, 0, 0x00, 0x00);  // Null
	gdt[1] = create_gdt_gate(0, 0, 0x9A, 0xA0);  // Code: exec/read, ring 0
	gdt[2] = create_gdt_gate(0, 0, 0x92, 0xA0);  // Data: read/write, ring 0
	gdt[3] = create_gdt_gate(0, 0, 0xFA, 0xA0);  // Code: user
	gdt[4] = create_gdt_gate(0, 0, 0xF2, 0xA0);  // Data: user

    gdt_load();
}