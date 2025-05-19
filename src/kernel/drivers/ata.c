#include <stdio.h>
#include <stdint.h>

#include <kernel/drivers/ata.h>
#include <kernel/utils/io.h>

#define PRIMARY 1
#define SECONDARY 2

#define ATA_PRIMARY_BASE		0x1F0	/* ATA bus */
#define ATA_PRIMARY_CTRL		0x3F6	/* Control */
#define ATA_PRIMARY_MASTER		0xA0	/* Channel */
#define ATA_PRIMARY_SLAVE		0xB0	/* Channel */

#define ATA_SECONDARY_BASE		0x170	/* ATA bus */
#define ATA_SECONDARY_CTRL		0x376	/* Control */
#define ATA_SECONDARY_MASTER	0xE0	/* Secondary channel */
#define ATA_SECONDARY_SLAVE		0xF0	/* Secondary channel */

#define ATA_REG_DATA			0x00	/* Data register (R/W) */
#define ATA_REG_ERROR			0x02	/* Error register (R) */
#define ATA_REG_SECCOUNT0		0x02	/* Sector count (R/W) */
#define ATA_REG_LBA0			0x03	/* LBA low byte (R/W) */
#define ATA_REG_LBA1			0x04	/* LBA mid byte (R/W) */
#define ATA_REG_LBA2			0x05	/* LBA high byte (R/W) */
#define ATA_REG_DRIVE			0x06	/* Drive/head register (R/W) */
#define ATA_REG_COMMAND			0x07	/* Command register (W) */
#define ATA_REG_STATUS			0x07	/* Status register (R) */

#define ATA_CMD_WRITE_SECT		0x30	/* Write sector */
#define ATA_CMD_READ_SECT		0x20	/* Read sector */
#define CMD_IDENTIFY			0xEC	/* Identify Device */

/* Status: */
#define ATA_SR_BSY				0x80	/* Busy */
#define ATA_SR_DRQ				0x08	/* Data Request Ready */
#define ATA_SR_ERR				0x01	/* Error */

/* Drive/head register bits */
#define ATA_DRIVE_MASTER_BASE	0xA0	/* Base value for master drive select (bit 7:1) */
#define ATA_DRIVE_SLAVE_BIT		0x10	/* Bit 4 set = slave drive */
#define ATA_LBA_BIT				0x40	/* Bit 6 set = LBA mode enabled */


#define MODEL_NAME_SIZE			41

uint16_t base[] = {0, ATA_PRIMARY_BASE, ATA_SECONDARY_BASE};
uint16_t ctrl[] = {0, ATA_PRIMARY_CTRL, ATA_SECONDARY_CTRL};

char primary_model[MODEL_NAME_SIZE];
char secondary_model[MODEL_NAME_SIZE];
char *model;

static int ata_wait(uint16_t io_base, uint8_t mask, int set) 
{
	for (int i = 0; i < 100000; ++i) 
	{
		uint8_t status = inb(io_base + ATA_REG_STATUS);
		if (set) {
			if ((status & mask) == mask)
				return 0;
		} else {
			if ((status & mask) == 0)
				return 0;
		}
	}
	return -1;
}

static inline void ata_select(uint8_t drive_id)
{
	if (drive_id != PRIMARY && drive_id != SECONDARY) 
	{
		debug_printf("Invalid drive_id: %u", drive_id);
		return;
	}

	outb(base[drive_id] + ATA_REG_DRIVE, 0xA0);

	switch (drive_id) 
	{
		case PRIMARY:
			model = primary_model;
			break;
		case SECONDARY:
			model = secondary_model;
			break;
	}

	io_wait();
}

uint16_t ata_identify(uint8_t drive_id)
{
	debug_printf("Checking HDD...");
	if (drive_id < 1 || drive_id > 2) 
	{
		debug_printf("Invalid drive_id: %u", drive_id);
		return 0;
	}

	ata_select(drive_id);

	/* Send some dummy I/O to the control register*/
	inb(ctrl[drive_id]);
	inb(ctrl[drive_id]);
	inb(ctrl[drive_id]);
	inb(ctrl[drive_id]);

	outb(base[drive_id] + ATA_REG_COMMAND, CMD_IDENTIFY);

	if (ata_wait(base[drive_id], ATA_SR_BSY, 0) != 0)
		return 0;

	// uint16_t identify_data[256];
	// for (int i = 0; i < 256; ++i) 
	// {
	// 	identify_data[i] = inw(base[drive_id]);
	// }

	// uint16_t cylinders = identify_data[1];
	// uint16_t heads = identify_data[3];
	// uint16_t sectors = identify_data[6];

	// uint32_t total_sectors = (uint32_t)cylinders * heads * sectors;
	// uint64_t total_bytes = (uint64_t)total_sectors * 512;

	for (int i = 0; i < 20; i++) 
	{
		model[i * 2] = identify_data[27 + i] >> 8;
		model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
	}

	model[40] = '\0';

	return 0;
}

int ata_write_sector(uint8_t drive_id, uint32_t lba, const char *buffer)
{
	uint16_t ata = base[drive_id];
	uint8_t lba_mode = ((lba >> 24) & 0x0F); /* First 4 bits */

	/* Select the drive and set the LBA mode*/
	outb(ata + ATA_REG_DRIVE, 0xA0 | lba_mode);

	/* Set the sector count (1 sector)*/
	outb(ata + ATA_REG_SECCOUNT0, 1);

	/* Send the LBA address (24 bits)*/
	outb(ata + ATA_REG_LBA0, lba & 0xFF);
	outb(ata + ATA_REG_LBA1, (lba >> 8) & 0xFF);
	outb(ata + ATA_REG_LBA2, (lba >> 16) & 0xFF);

	/* Send the WRITE SECTOR command*/
	outb(ata + ATA_REG_COMMAND, ATA_CMD_WRITE_SECT);

	if (ata_wait(ata, ATA_SR_DRQ, 1) != 0)
		return -1;

	/* Write 512 bytes as a word(u16) */
	for (int i = 0; i < 256; ++i)
	{
		outw(ata + ATA_REG_DATA, ((const uint16_t *)buffer)[i]);
	}

	/* Wait for the BSY bit to clear (operation complete)*/
	if (ata_wait(ata, ATA_SR_BSY, 0) != 0)
		return -1;

	debug_printf("Wrote sector %u\n", lba);

	return 0;
}

int ata_read_sector(uint8_t drive_id, uint32_t lba, char *buffer) 
{
	if (drive_id != PRIMARY && drive_id != SECONDARY)
		return -1;

	uint16_t io = base[drive_id];
	uint8_t lba_high = (lba >> 24) & 0x0F;

	outb(io + ATA_REG_DRIVE, 0xE0 | lba_high);
	outb(io + ATA_REG_SECCOUNT0, 1);
	outb(io + ATA_REG_LBA0, lba & 0xFF);
	outb(io + ATA_REG_LBA1, (lba >> 8) & 0xFF);
	outb(io + ATA_REG_LBA2, (lba >> 16) & 0xFF);

	outb(io + ATA_REG_COMMAND, ATA_CMD_READ_SECT);

	if (ata_wait(io, ATA_SR_BSY, 0) != 0)
		return -1;

	if (ata_wait(io, ATA_SR_DRQ, 1) != 0)
		return -1;

	for (int i = 0; i < 256; ++i)
		((uint16_t*)buffer)[i] = inw(io + ATA_REG_DATA);

	return 0;
}
