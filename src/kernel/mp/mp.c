#include <stdint.h>
#include <string.h>
#include <kernel/mp/mp.h>

static unsigned char sum(unsigned char *addr, int len)
{
	int i, sum;

	sum = 0;
	for(i=0; i<len; i++)
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
	
	return 0;
}

static struct mp* mp_search(void)
{
	unsigned char *bda;
	unsigned int ptr;
	struct mp *mp;

	// Base Data Area
	bda = 0x400;


    // First check: Look at the floppy drive information in the BDA
    // The bytes at offsets 0x0F and 0x0E hold the physical address (multiplied by 16) of the MP floating pointer structure
    // The offset 0x0F holds the high byte, and 0x0E holds the low byte

	ptr = ((bda[0x0F] << 8) | bda[0x0E]) << 4;

	if(ptr)
	{
		// If a valid address is found (non-zero), search within the first 1 KB of memory starting at that address
		if((mp = mp_search1(ptr, 1024)))
			return mp;
	}

	else 
	{
		// Second check: If no address was found, look for the physical address in memory starting at offset 0x14 and 0x13
        // These bytes hold the base address of the MP structure, in kilobytes (multiplied by 1024 to get the byte address)
		
		ptr = ((bda[0x14] << 8) | bda[0x13])  * 1024;
		if((mp = mp_search1(ptr-1024, 1024)))
			return mp;
	}

	// Fallback: Search in the BIOS ROM area (0xF0000 to 0xFFFFF)
	return mp_search1(0xF0000, 0x10000);
}