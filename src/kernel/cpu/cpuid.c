#include <stdint.h>
#include <stdbool.h>

#include <kernel/cpu/cpuid.h>
#include <kernel/terminal/tty.h>
#include <string.h>

typedef struct CPUID {
	char brand_name[13];
} CPUID;

CPUID cpu;

void cpuid(uint32_t code, uint32_t *output)
{
	__asm__ __volatile__(
		"cpuid"
		: "=a" (output[0]), "=b" (output[1]), "=c" (output[3]), "=d" (output[2])
		: "a" (code)
	);
}

void init_cpuid()
{
	cpuid_get_brand();
}

void cpuid_get_brand()
{
	uint32_t registers[4];
	unsigned eax = 0;

	cpuid(eax, &registers[0]);

	memcpy(&cpu.brand_name[0], &registers[1], 4);
	memcpy(&cpu.brand_name[4], &registers[2], 4);
	memcpy(&cpu.brand_name[8], &registers[3], 4);
	cpu.brand_name[12] = '\0';
}

int cpuid_get_feature(uint64_t feature_id)
{
	uint32_t registers[4];

	cpuid(0x01, &registers[0]);


	/**
	 * ecx = registers[2]
	 * edx = registers[3]
	 */
	return (feature_id & registers[2] & registers[3]);
}

bool check_apic()
{
	uint32_t output[4];

	cpuid(1, output);

	return output[3] & CPUID_FEAT_EDX_APIC;
}

void cpuid_kprint()
{
	terminal_writestring("Cpu brand: ");
	terminal_writestring(cpu.brand_name);
	terminal_writestring("\n");
}