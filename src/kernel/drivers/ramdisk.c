#include <stdint.h>
#include <string.h>
#include <kernel/drivers/ramdisk.h>
#include <kernel/mem/kmalloc.h>

#define SECTOR_SIZE 512

uint8_t *ram_disk;

void ramdisk_init(uint32_t sector_ammount)
{
	uint32_t num_bytes = sector_ammount * SECTOR_SIZE;

	ram_disk = kmalloc(num_bytes);
	memset(ram_disk, num_bytes, 0);
}

uint8_t *ramdisk_read_sector(uint32_t sector)
{	
	sector++;
	uint8_t *sector_addr = &ram_disk[sector * SECTOR_SIZE];

	uint8_t *buffer = kmalloc(SECTOR_SIZE);

	memcpy(buffer, sector_addr, SECTOR_SIZE);

	return sector_addr;
}

void ramdisk_write_sector(uint8_t *sector_data, uint32_t sector)
{	
	sector++;
	uint8_t *sector_addr = &ram_disk[sector * SECTOR_SIZE];

	memcpy(sector_addr, sector_data, SECTOR_SIZE);
}