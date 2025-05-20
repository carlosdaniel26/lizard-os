#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <kernel/drivers/ata.h>
#include <kernel/utils/io.h>
#include <kernel/arch/idt.h>

uint16_t base[] = {ATA_PRIMARY_BASE, ATA_SECONDARY_BASE};
uint16_t ctrl[] = {ATA_PRIMARY_CTRL, ATA_SECONDARY_CTRL};

ATADevice ata_devices[2];

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

void unmask_ata_primary_irq() 
{
    uint8_t mask = inb(PIC2_DATA);

    mask &= ~(1 << 6);

    outb(PIC2_DATA, mask);
}

static int ata_wait(uint16_t io_base, uint8_t mask, int set) 
{
	for (int i = 0; i < 100000; ++i) {
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

static inline void ata_select(ATADevice *dev)
{
	outb(dev->io_base + ATA_REG_DRIVE, 0xA0);

	io_wait();
}

void ata_print_devices() 
{
	for (int i = 0; i < 2; i++) {
		if (ata_devices[i].present)
			debug_printf("Drive %u: %s\n", i, ata_devices[i].model);
		else
			debug_printf("Drive %u: absent\n", i);
	}
}

void ata_detect_devices()
{
	for (uint8_t i = PRIMARY; i <= SECONDARY; i++)
	{
		ata_devices[i].id = i;
		ata_devices[i].io_base = base[i];
		ata_devices[i].ctrl_base = ctrl[i];
		
		if (ata_identify(&ata_devices[i]) == 0)
		{
			ata_devices[i].present = 1;
		}
		else
		{
			ata_devices[i].present = 0;
		}
	}

	outb(ata_devices[0].io_base + ATA_REG_COMMAND, 0x00);
	outb(ata_devices[1].io_base + ATA_REG_COMMAND, 0x00);

	create_idt_descriptor(46, stub_46, 0x8E);
	create_idt_descriptor(47, stub_47, 0x8E);

	unmask_ata_primary_irq();
}

int ata_identify(ATADevice *dev)
{
	ata_select(dev);

	/* Send some dummy I/O to the control register*/
	inb(dev->io_base);
	inb(dev->io_base);
	inb(dev->io_base);
	inb(dev->io_base);

	outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

	if (ata_wait(dev->io_base, ATA_SR_BSY, 0) != 0)
		return -1;

	if (ata_wait(dev->io_base, ATA_SR_DRQ, 1) != 0)
		return -1;

	uint16_t identify_data[256];
	for (int i = 0; i < 256; ++i) {
		identify_data[i] = inw(dev->io_base);
	}

	// uint16_t cylinders = identify_data[1];
	// uint16_t heads = identify_data[3];
	// uint16_t sectors = identify_data[6];

	// uint32_t total_sectors = (uint32_t)cylinders * heads * sectors;
	// uint64_t total_bytes = (uint64_t)total_sectors * 512;

	for (int i = 0; i < 20; i++) {
		dev->model[i * 2] = identify_data[27 + i] >> 8;
		dev->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
	}

	dev->model[40] = '\0';

	return 0;
}

int atapio_write_sector(ATADevice *dev, uint32_t lba, const char *buffer)
{
	debug_printf("WRITING ON DISK %u", dev->id);
	uint16_t ata = dev->io_base;
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
		outw(ata + ATA_REG_DATA, ((const uint16_t *)buffer)[i]);

	/* Wait for the BSY bit to clear (operation complete)*/
	if (ata_wait(ata, ATA_SR_BSY, 0) != 0)
		return -1;

	return 0;
}

int atapio_read_sector(ATADevice *dev, uint32_t lba, char *buffer) 
{
	debug_printf("READING ON DISK %u", dev->id);
	uint16_t ata = dev->io_base;

	uint8_t lba_high = (lba >> 24) & 0x0F; /* First 4 bits */

	/* Select the drive and set the LBA mode*/
	outb(ata + ATA_REG_DRIVE, 0xE0 | lba_high);

	/* Set the sector count (1 sector)*/
	outb(ata + ATA_REG_SECCOUNT0, 1);

	/* Send the LBA address (24 bits)*/
	outb(ata + ATA_REG_LBA0, lba & 0xFF);
	outb(ata + ATA_REG_LBA1, (lba >> 8) & 0xFF);
	outb(ata + ATA_REG_LBA2, (lba >> 16) & 0xFF);

	/* Send the WRITE SECTOR command*/
	outb(ata + ATA_REG_COMMAND, ATA_CMD_READ_SECT);

	if (ata_wait(ata, ATA_SR_BSY, 0) != 0)
		return -1;

	if (ata_wait(ata, ATA_SR_DRQ, 1) != 0)
		return -1;

	/* Read 512 bytes as a word(u16) */
	for (int i = 0; i < 256; ++i)
		((uint16_t*)buffer)[i] = inw(ata + ATA_REG_DATA);

	return 0;
}

ATADevice *ata_get(uint8_t drive_id)
{
	if (drive_id <= 2)
	{
		return ata_devices[drive_id];
	}

	return NULL;
}

static inline void ata_general_isr(ATADevice *dev)
{

}

void isr_ata_primary()
{
	ata_general_isr(&ata_devices[PRIMARY]);
}

void isr_ata_secondary()
{
	ata_general_isr(&ata_devices[SECONDARY]);
}