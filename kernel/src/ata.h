#pragma once

#include <stdint.h>

#define PRIMARY 0
#define SECONDARY 1

/* Ports */
#define ATA_PRIMARY_BASE 0x1F0	/* ATA bus */
#define ATA_PRIMARY_CTRL 0x3F6	/* Control */
#define ATA_PRIMARY_MASTER 0xA0 /* Channel */
#define ATA_PRIMARY_SLAVE 0xB0	/* Channel */

#define ATA_SECONDARY_BASE 0x170  /* ATA bus */
#define ATA_SECONDARY_CTRL 0x376  /* Control */
#define ATA_SECONDARY_MASTER 0xE0 /* Secondary channel */
#define ATA_SECONDARY_SLAVE 0xF0  /* Secondary channel */

/* Registers Offsets */
#define ATA_REG_DATA 0x00	   /* Data register (R/W) */
#define ATA_REG_ERROR 0x02	   /* Error register (R) */
#define ATA_REG_SECCOUNT0 0x02 /* Sector count (R/W) */
#define ATA_REG_LBA0 0x03	   /* LBA low byte (R/W) */
#define ATA_REG_LBA1 0x04	   /* LBA mid byte (R/W) */
#define ATA_REG_LBA2 0x05	   /* LBA high byte (R/W) */
#define ATA_REG_DRIVE 0x06	   /* Drive/head register (R/W) */
#define ATA_REG_COMMAND 0x07   /* Command register (W) */
#define ATA_REG_STATUS 0x07	   /* Status register (R) */

/* Commands */
#define ATA_CMD_WRITE_SECT 0x30 /* Write sector */
#define ATA_CMD_READ_SECT 0x20	/* Read sector */
#define ATA_CMD_IDENTIFY 0xEC	/* Identify Device */

/* Status: */
#define ATA_SR_BSY 0x80 /* Busy */
#define ATA_SR_DRQ 0x08 /* Data Request Ready */
#define ATA_SR_ERR 0x01 /* Error */

/* Drive/head register bits */
#define ATA_DRIVE_MASTER_BASE 0xA0 /* Base value for master drive select (bit 7:1) */
#define ATA_DRIVE_SLAVE_BIT 0x10   /* Bit 4 set = slave drive */
#define ATA_LBA_BIT 0x40		   /* Bit 6 set = LBA mode enabled */

/* General Values */
#define MODEL_NAME_SIZE 41

#define ATA_DEFAULT_SECTOR_SIZE 512

typedef struct ATADevice {
	uint8_t id;
	uint16_t io_base;
	uint16_t ctrl_base;
	uint8_t present;

	char model[MODEL_NAME_SIZE];
	uint16_t cylinders;
	uint16_t heads;
	uint16_t sectors;
	uint16_t sector_size;

	uint32_t total_sectors;
	uint64_t total_bytes;
} ATADevice;

void ata_detect_devices();

void isr_ata_primary();
void isr_ata_secondary();

