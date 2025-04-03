#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <kernel/mp/mp.h>

static unsigned int sum(unsigned char *addr, int len)
{
	int i, sum = 0;
	for(i = 0; i < len; i++)
		sum += addr[i];

	return sum;
}

static struct mp* mp_search1(unsigned a, int len)
{
	unsigned char *end, *ptr, *addr;
	addr = (unsigned char *)a;
	end = addr + len;

	for (ptr = addr; ptr < end; ptr += sizeof(struct mp))
	{
		if (memcmp(ptr, "_MP_", 4) == 0 && sum(ptr, sizeof(struct mp)) == 0)
			return (struct mp *)ptr;
	}
	
	return NULL;
}

static struct mp* mp_search(void)
{
	unsigned char *bda;
	unsigned int ptr;
	struct mp *mp;

	// Correctly cast the Base Data Area pointer
	bda = (unsigned char *)0x400;

	// First check: Look at the floppy drive information in the BDA
	ptr = ((bda[0x0F] << 8) | bda[0x0E]) << 4;

	if (ptr)
	{
		// If a valid address is found, search within the first 1 KB of memory
		if ((mp = mp_search1(ptr, 1024)))
			return mp;
	}
	else
	{
		// Second check: Look for base address in memory offsets 0x14 and 0x13
		ptr = ((bda[0x14] << 8) | bda[0x13]) * 1024;
		if ((mp = mp_search1(ptr - 1024, 1024)))
			return mp;
	
	}
	// Fallback: Search in the BIOS ROM area (0xF0000 to 0xFFFFF)
	return mp_search1(0xF0000, 0x10000);
}

static struct mpconfig *mpconfig(struct mp **mp_ptr)
{
	struct mpconfig *config;
	struct mp *mp;

	if ((mp = mp_search()) == NULL || mp->physaddr == 0)
	{
		kprintf("MP not found\n");
		return NULL;
	}

	config = (struct mpconfig *)(mp->physaddr);

	// Validate MP configuration
	if (memcmp(config, "PCMP", 4) != 0)
		return NULL;
	if (config->version != 1 && config->version != 4)
		return NULL;
	if (sum((unsigned char*)config, config->length) != 0)
		return NULL;

	// Store found MP pointer only if `mp_ptr` is not NULL
	if (mp_ptr)
		*mp_ptr = mp;

	return config;
}

void mp_init(void)
{
	struct mp *mp;
	struct mpconfig *config;

	if ((config = mpconfig(&mp)) == NULL)
	{
		kprintf("MP config not found\n");
		return;
	}
}
