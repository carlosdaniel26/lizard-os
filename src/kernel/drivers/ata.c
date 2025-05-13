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

static inline void ata_select(uint8_t drive_id)
{
	if (drive_id != PRIMARY || drive_id != SECONDARY)
	{
		debug_printf("Invalid drive_id: %u\n", drive_id);
		return;
	}

	outb(base[drive_id] + ATA_REG_DRIVE, 0xB0);

}

uint16_t ata_identify(uint8_t drive_id)
{
    if (drive_id < 1 || drive_id > 2) {
        debug_printf("invalid drive_id: %u\n", drive_id);
        return 0;
    }

    ata_select(drive_id);

    inb(ctrl[drive_id]);
    inb(ctrl[drive_id]);
    inb(ctrl[drive_id]);
    inb(ctrl[drive_id]);

    outb(base[drive_id] + ATA_REG_COMMAND, CMD_IDENTIFY);

    uint8_t status = 0x00;
    
    while (status & 0x80); 
    {
        status = inb(base[drive_id] + ATA_REG_STATUS);
    }

    return status;
}
