#include <stdio.h>
#include <stdint.h>
#include <kernel/utils/io.h>

#define PRIMARY 1
#define SECONDARY 2

#define ATA_PRIMARY_BASE 0x1F0
#define ATA_PRIMARY_CTRL 0x3F6

#define ATA_SECONDARY_BASE   0x170
#define ATA_SECONDARY_CTRL   0x376

#define ATA_REG_DRIVE 6
#define ATA_REG_COMMAND 7
#define ATA_REG_STATUS 7

#define CMD_IDENTIFY 0xEC

uint16_t base[] = {0, ATA_PRIMARY_BASE, ATA_SECONDARY_BASE};
uint16_t ctrl[] = {0, ATA_PRIMARY_CTRL, ATA_SECONDARY_CTRL};

#define MODEL_NAME_SIZE 41

char primary_model[MODEL_NAME_SIZE];
char secondary_model[MODEL_NAME_SIZE];
char *model;

static inline void ata_select(uint8_t drive_id)
{
	if (drive_id != PRIMARY && drive_id != SECONDARY)
	{
		debug_printf("Invalid drive_id: %u\n", drive_id);
		return;
	}

	outb(base[drive_id] + ATA_REG_DRIVE, 0xA0);

	switch(drive_id)
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
	if (drive_id < 1 || drive_id > 2) {
		debug_printf("invalid drive_id: %u", drive_id);
		return 0;
	}

	ata_select(drive_id);

	inb(ctrl[drive_id]);
	inb(ctrl[drive_id]);
	inb(ctrl[drive_id]);
	inb(ctrl[drive_id]);

	outb(base[drive_id] + ATA_REG_COMMAND, CMD_IDENTIFY);

	uint8_t status;

	do {
		status = inb(base[drive_id] + ATA_REG_STATUS);
	} while (status & 0x80);

	if (status & 0x01) {
		debug_printf("ATA Error on drive %u", drive_id);
		return 0;
	}

	if (!(status & 0x08)) {
		debug_printf("DRQ not set; device not ready");
		return 0;
	}

	uint16_t identify_data[256];
	for (int i = 0; i < 256; ++i)
	{
		identify_data[i] = inw(base[drive_id]);
	}

	uint16_t cylinders = identify_data[1];
	uint16_t heads	 = identify_data[3];
	uint16_t sectors   = identify_data[6];

	uint32_t total_sectors = (uint32_t)cylinders * heads * sectors;
	uint64_t total_bytes = (uint64_t)total_sectors * 512;


	debug_printf("Cylinders: %u", cylinders);
	debug_printf("Head: %u", heads);
	debug_printf("Sectors: %u", sectors);

	debug_printf("Storage Capacity: %uMB", total_bytes / (1024 * 1024));
	for (int i = 0; i < 20; i++)
	{
		model[i * 2] = identify_data[27 + i] >> 8;
		model[i*2 + 1] = identify_data[27 + i] & 0xFF;
	}
	model[40] = '\0';

	debug_printf("Drive model: %s\n", model);


	return status;
}
