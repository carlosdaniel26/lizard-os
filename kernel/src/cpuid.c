#include <cpuid.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <tty.h>

CPUID cpu;

void cpuid(uint32_t code, uint32_t *output)
{
	__asm__ __volatile__("cpuid"
						 : "=a"(output[0]), "=b"(output[1]), "=c"(output[2]), "=d"(output[3])
						 : "a"(code));
}

void init_cpuid()
{
	cpuid_get_brand();
}

void cpuid_get_brand()
{
	uint32_t regs[4];
	char *brand = cpu.brand_name;

	for (uint32_t i = 0; i < 3; i++)
	{
		cpuid(0x80000002 + i, regs);

		memcpy(brand + i * 16 + 0, &regs[0], 4);
		memcpy(brand + i * 16 + 4, &regs[1], 4);
		memcpy(brand + i * 16 + 8, &regs[2], 4);
		memcpy(brand + i * 16 + 12, &regs[3], 4);
	}

	brand[48] = '\0';
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
	tty_writestring("Cpu brand: ");
	tty_writestring(cpu.brand_name);
	tty_writestring("\n");
}