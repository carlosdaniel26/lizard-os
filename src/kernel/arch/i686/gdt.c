#include <stdint.h>
#include <kernel/arch/gdt.h>

gdt_entry_struct gdt_entry[5];

gdt_ptr_struct gdt_ptr;

void set_gdt_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
	// Set the base address
	gdt_entry[num].base_low	= (base & 0xFFFF);
	gdt_entry[num].base_middle = (base >> 16) & 0xFF;
	gdt_entry[num].base_high   = (base >> 24) & 0xFF;

	// Set the limit
	gdt_entry[num].limit_low   = (limit & 0xFFFF);
	gdt_entry[num].granularity = ((limit >> 16) & 0x0F);
	gdt_entry[num].granularity |= (granularity & 0xF0);
	gdt_entry[num].access	  = access;
}

static inline void gdt_load()
{
	gdt_ptr.limit = (sizeof(gdt_entry_struct) * 5) - 1;
	gdt_ptr.base = (uint32_t)&gdt_entry;

	// Load the GDT
	asm volatile("lgdt %0" : : "m"(gdt_ptr));
	asm volatile("mov $0x10, %%ax; \
				  mov %%ax, %%ds; \
				  mov %%ax, %%es; \
				  mov %%ax, %%fs; \
				  mov %%ax, %%gs; \
				  ljmp $0x08, $next; \
				  next:": : : "eax");
}

void init_gdt()
{
	set_gdt_gate(0, 0, 0, 0, 0);

	set_gdt_gate(0, 0, 0, 0, 0);				// Null segment
	set_gdt_gate(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Kernel code segment
	set_gdt_gate(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Kernel data segment
	set_gdt_gate(3, 0, 0xFFFFFFFF, 0xFA, 0xCF); // User code segment
	set_gdt_gate(4, 0, 0xFFFFFFFF, 0xF2, 0xCF); // User data segment

	gdt_load();
}