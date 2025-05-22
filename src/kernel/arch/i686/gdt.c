#ifdef TARGET_I686
#include <stdint.h>
#include <kernel/arch/gdt.h>

global_descriptor gdt_entry[5];

gdt_ptr gdt_pointer;

global_descriptor create_gdt_gate(uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity)
{
	global_descriptor gdt_element;

	/* Set the base address*/
	gdt_element.base_low	= (base & 0xFFFF);
	gdt_element.base_middle = (base >> 16) & 0xFF;
	gdt_element.base_high   = (base >> 24) & 0xFF;

	/* Set the limit*/
	gdt_element.limit_low   = (limit & 0xFFFF);
	gdt_element.granularity = ((limit >> 16) & 0x0F);
	gdt_element.granularity |= (granularity & 0xF0);
	gdt_element.access	  = access;

	return gdt_element;
}

static inline void gdt_load()
{
	gdt_pointer.limit = (sizeof(global_descriptor) * 5) - 1;
	gdt_pointer	.base = (uint32_t)&gdt_entry;

	/* Load the GDT*/
	asm volatile("lgdt %0" : : "m"(gdt_pointer));
	asm volatile("mov $0x10, %%ax; \
				  mov %%ax, %%ds; \
				  mov %%ax, %%es; \
				  mov %%ax, %%fs; \
				  mov %%ax, %%gs; \
				  mov %%ax, %%ss; \
				  ljmp $0x08, $next; \
				  next:": : : "eax");
}

void init_gdt()
{
	gdt_entry[0] = create_gdt_gate(0, 0, 0, 0);					/* Null segment*/
	gdt_entry[1] = create_gdt_gate(0, 0xFFFFFFFF, 0x9A, 0xCF); 	/* Kernel code segment*/
	gdt_entry[2] = create_gdt_gate(0, 0xFFFFFFFF, 0x92, 0xCF); 	/* Kernel data segment*/
	gdt_entry[3] = create_gdt_gate(0, 0xFFFFFFFF, 0xFA, 0xCF); 	/* User code segment*/
	gdt_entry[4] = create_gdt_gate(0, 0xFFFFFFFF, 0xF2, 0xCF); 	/* User data segment*/

	gdt_load();
}
#endif