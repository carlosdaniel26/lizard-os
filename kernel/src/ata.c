#include <ata.h>
#include <io.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <block_dev.h>
#include <kmalloc.h>

uint16_t base[] = {ATA_PRIMARY_BASE, ATA_SECONDARY_BASE};
uint16_t ctrl[] = {ATA_PRIMARY_CTRL, ATA_SECONDARY_CTRL};

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

static int block_read(BlockDevice *dev, uint64_t sector, void *buffer, size_t count);
static int block_write(BlockDevice *dev, uint64_t sector, void *buffer, size_t count);
static int block_flush(BlockDevice *dev);

BlockDeviceOps ata_block_ops = {
	.read = block_read,
	.write = block_write,
	.flush = block_flush
};

void unmask_ata_primary_irq()
{
	uint8_t mask = inb(PIC2_DATA);

	mask &= ~(1 << 6);

	outb(PIC2_DATA, mask);
}

static int ata_wait(uint16_t io_base, uint8_t mask, int set)
{
	for (int i = 0; i < 100000; ++i)
	{
		uint8_t status = inb(io_base + ATA_REG_STATUS);
		if (set)
		{
			if ((status & mask) == mask)
				return 0;
		} else
		{
			if ((status & mask) == 0)
				return 0;
		}
	}
	return -1;
}

static inline void ata_select(ATADevice *dev)
{
	outb(dev->io_base + ATA_REG_DRIVE, 0xA0);

	io_wait();
}

void ata_detect_devices()
{
	for (uint8_t i = PRIMARY; i <= PRIMARY; i++)
	{
		ATADevice *ata_dev = kcalloc(sizeof(ATADevice));

		ata_dev->id = i;
		ata_dev->io_base = base[i];
		ata_dev->ctrl_base = ctrl[i];

		ata_select(ata_dev);

		outb(ata_dev->io_base + ATA_REG_SECCOUNT0, 0);
		outb(ata_dev->io_base + ATA_REG_LBA0, 0);
		outb(ata_dev->io_base + ATA_REG_LBA1, 0);
		outb(ata_dev->io_base + ATA_REG_LBA2, 0);
		outb(ata_dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

		uint8_t status = inb(ata_dev->io_base + ATA_REG_STATUS);
		if (status == 0) {
			kfree(ata_dev);
			continue;
		}

		if (ata_wait(ata_dev->io_base, ATA_SR_BSY, 0) != 0) {
			kfree(ata_dev);
			continue;
		}

		if (ata_wait(ata_dev->io_base, ATA_SR_ERR, 0) != 0) {
			kfree(ata_dev);
			continue;
		}

		uint16_t identify_data[256];
		for (int i = 0; i < 256; ++i)
			identify_data[i] = inw(ata_dev->io_base);

		if (identify_data[0] & (1 << 15)) 
		{
			kfree(ata_dev);
			continue;
		}

		ata_dev->present = 1;
		ata_dev->cylinders = identify_data[1];
		ata_dev->heads = identify_data[3];
		ata_dev->sectors = identify_data[6];

		if (identify_data[83] & (1 << 10)) 
		{
			ata_dev->total_sectors = (uint16_t)identify_data[60];
		} 
		else 
		{
			ata_dev->total_sectors = (uint32_t)ata_dev->cylinders * ata_dev->heads * ata_dev->sectors;
		}
		
		ata_dev->total_bytes = (uint64_t)ata_dev->total_sectors * 512;

		/* Get Sector Size */
		if (identify_data[106] & (1 << 14))
		{
			ata_dev->sector_size = *(uint16_t*)&identify_data[117];
		}
		else
		{
			ata_dev->sector_size = ATA_DEFAULT_SECTOR_SIZE;
		}

		for (int i = 0; i < 20; i++) 
		{
			ata_dev->model[i * 2] = identify_data[27 + i] >> 8;
			ata_dev->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
		}
		ata_dev->model[40] = '\0';
		
		BlockDevice *dev = kcalloc(sizeof(BlockDevice));
		strcpy(dev->name, ata_dev->model);
		dev->id = i;
		dev->total_sectors = ata_dev->total_sectors;
		dev->sector_size = ata_dev->sector_size;
		dev->ops = &ata_block_ops;
		dev->private_data = (void*)ata_dev;
		dev->initialized = true;
		dev->read_only = false;
		dev->present = true;

		block_device_register(dev);
	}
}

/* Block Device */
static int block_read(BlockDevice *dev, uint64_t sector, void *buffer, size_t count)
{
	ATADevice *ata_dev = (ATADevice*)dev->private_data;

	uint64_t end_sector = sector + count;

	uint16_t ata = ata_dev->io_base;

	for (uint64_t s = sector; s < end_sector; s++, buffer += ata_dev->sector_size)
	{
		uint8_t lba_high = (sector >> 24) & 0x0F; /* First 4 bits */

		/* Select the drive and set the LBA mode*/
		outb(ata + ATA_REG_DRIVE, 0xE0 | lba_high);

		/* Set the sector count (1 sector)*/
		outb(ata + ATA_REG_SECCOUNT0, 1);

		/* Send the LBA address (24 bits)*/
		outb(ata + ATA_REG_LBA0, sector & 0xFF);
		outb(ata + ATA_REG_LBA1, (sector >> 8) & 0xFF);
		outb(ata + ATA_REG_LBA2, (sector >> 16) & 0xFF);

		/* Send the WRITE SECTOR command*/
		outb(ata + ATA_REG_COMMAND, ATA_CMD_READ_SECT);

		if (ata_wait(ata, ATA_SR_BSY, 0) != 0)
			return -1;

		if (ata_wait(ata, ATA_SR_DRQ, 1) != 0)
			return -1;

		/* Read 512 bytes as a word(u16) */
		for (int i = 0; i < 256; ++i)
			((uint16_t *)buffer)[i] = inw(ata + ATA_REG_DATA);
	}

	return 0;
}

static int block_write(BlockDevice *dev, uint64_t sector, void *buffer, size_t count)
{
	ATADevice *ata_dev = (ATADevice*)dev->private_data;

	uint64_t end_sector = sector + count;

	uint16_t ata = ata_dev->io_base;

	for (uint64_t s = sector; s < end_sector; s++, buffer += ata_dev->sector_size)
	{
		uint8_t lba_high = (sector >> 24) & 0x0F; /* First 4 bits */

		/* Select the drive and set the LBA mode*/
		outb(ata + ATA_REG_DRIVE, 0xE0 | lba_high);

		/* Set the sector count (1 sector)*/
		outb(ata + ATA_REG_SECCOUNT0, 1);

		/* Send the LBA address (24 bits)*/
		outb(ata + ATA_REG_LBA0, sector & 0xFF);
		outb(ata + ATA_REG_LBA1, (sector >> 8) & 0xFF);
		outb(ata + ATA_REG_LBA2, (sector >> 16) & 0xFF);

		/* Send the READ SECTOR command*/
		outb(ata + ATA_REG_COMMAND, ATA_CMD_WRITE_SECT);

		if (ata_wait(ata, ATA_SR_BSY, 0) != 0)
			return -1;

		if (ata_wait(ata, ATA_SR_DRQ, 1) != 0)
			return -1;

		/* Write 512 bytes as a word(u16) */
		for (int i = 0; i < 256; ++i)
			outw(ata + ATA_REG_DATA, ((uint16_t *)buffer)[i]);
	}

	return 0;
}

static int block_flush(BlockDevice *dev)
{
	(void)dev;
	return 0;
}

// static inline void ata_general_isr(ATADevice *dev)
// {

// }

// void isr_ata_primary()
// {
//	ata_general_isr(&ata_devices[PRIMARY]);
// }

// void isr_ata_secondary()
// {
//	ata_general_isr(&ata_devices[SECONDARY]);
// }