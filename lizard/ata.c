#include <lizard/ata.h>
#include <lizard/blk_dev.h>
#include <lizard/init.h>
#include <lizard/io.h>
#include <lizard/kmalloc.h>
#include <lizard/setup.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <nolibc/types.h>

static u16 base[] = {ATA_PRIMARY_BASE, ATA_SECONDARY_BASE};
static u16 ctrl[] = {ATA_PRIMARY_CTRL, ATA_SECONDARY_CTRL};

#define PIC1_DATA 0x21
#define PIC2_DATA 0xA1

static int block_read(struct block_dev *dev, u64 sector, void *buffer, size_t count);
static int block_write(struct block_dev *dev, u64 sector, void *buffer, size_t count);
static int block_flush(struct block_dev *dev);

static struct block_dev_ops ata_block_ops = {.read = block_read, .write = block_write, .flush = block_flush};

// static void unmask_ata_primary_irq()
// {
// 	u8 mask = inb(PIC2_DATA);

// 	mask &= ~(1 << 6);

// 	outb(PIC2_DATA, mask);
// }

static int ata_wait(u16 io_base, u8 mask, int set)
{
    for (int i = 0; i < 100000; ++i)
    {
        u8 status = inb(io_base + ATA_REG_STATUS);
        if (set)
        {
            if ((status & mask) == mask) return 0;
        }
        else
        {
            if ((status & mask) == 0) return 0;
        }
        io_wait();
    }
    return -1;
}

static inline void ata_select(struct ata_device *dev)
{
    outb(dev->io_base + ATA_REG_DRIVE, 0xA0);

    io_wait();
}

int ata_detect_devices()
{
    for (u8 ata_id = PRIMARY; ata_id <= PRIMARY; ata_id++)
    {
        struct ata_device *ata_dev = zalloc(sizeof(struct ata_device));

        ata_dev->io_base = base[ata_id];
        ata_dev->ctrl_base = ctrl[ata_id];

        ata_select(ata_dev);

        outb(ata_dev->io_base + ATA_REG_SECCOUNT0, 0);
        outb(ata_dev->io_base + ATA_REG_LBA0, 0);
        outb(ata_dev->io_base + ATA_REG_LBA1, 0);
        outb(ata_dev->io_base + ATA_REG_LBA2, 0);
        outb(ata_dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

        u8 status = inb(ata_dev->io_base + ATA_REG_STATUS);
        if (status == 0)
        {
            kfree(ata_dev);
            continue;
        }

        if (ata_wait(ata_dev->io_base, ATA_SR_BSY, 0) != 0)
        {
            kfree(ata_dev);
            continue;
        }

        if (ata_wait(ata_dev->io_base, ATA_SR_ERR, 0) != 0)
        {
            kfree(ata_dev);
            continue;
        }

        u16 identify_data[256];
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

        /* Total sectors: prefer LBA48 (words 100-103), then the full 32-bit
         * LBA28 value (words 60-61), then legacy CHS. Word 60 alone is only the
         * low 16 bits, so reading it in isolation truncates any disk that is a
         * multiple of 65536 sectors (e.g. 64 MiB) down to garbage. */
        u64 lba48 = (u64)identify_data[100] | ((u64)identify_data[101] << 16) |
                    ((u64)identify_data[102] << 32) | ((u64)identify_data[103] << 48);
        u32 lba28 = (u32)identify_data[60] | ((u32)identify_data[61] << 16);

        if ((identify_data[83] & (1 << 10)) && lba48)
        {
            ata_dev->total_sectors = lba48;
        }
        else if (lba28)
        {
            ata_dev->total_sectors = lba28;
        }
        else
        {
            ata_dev->total_sectors = (u32)ata_dev->cylinders * ata_dev->heads * ata_dev->sectors;
        }

        /* Get Sector Size */
        if (identify_data[117])
        {
            ata_dev->sector_size = *(u16 *)&identify_data[117];
        }
        else
        {
            ata_dev->sector_size = ATA_DEFAULT_SECTOR_SIZE;
        }

        ata_dev->total_bytes = (u64)ata_dev->total_sectors * ata_dev->sector_size;

        for (int i = 0; i < 20; i++)
        {
            ata_dev->model[i * 2] = identify_data[27 + i] >> 8;
            ata_dev->model[i * 2 + 1] = identify_data[27 + i] & 0xFF;
        }
        ata_dev->model[40] = '\0';

        char name[sizeof("ata0")];

        name[0] = 'a';
        name[1] = 't';
        name[2] = 'a';
        name[3] = '0' + ata_id; /* id must be 0 or 1 */
        name[4] = '\0';

        struct block_dev *dev = zalloc(sizeof(struct block_dev));
        strcpy(dev->name, name);
        dev->type = BLKDEV_TYPE_PHYSICAL;
        dev->total_sectors = ata_dev->total_sectors;
        dev->sector_size = ata_dev->sector_size;
        dev->ops = &ata_block_ops;
        dev->private_data = (void *)ata_dev;
        dev->initialized = true;
        dev->read_only = false;
        dev->present = true;

        dev->parent = NULL;
        InitListHead(&dev->children);
        InitListHead(&dev->siblings);

        blkdev_manager_add(dev);
    }

    return 0;
}

device_initcall(ata_detect_devices);

/* Block Device */
static void ata_setup_lba(u16 ata, u64 lba)
{
    outb(ata + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ata + ATA_REG_SECCOUNT0, 1);
    outb(ata + ATA_REG_LBA0, lba & 0xFF);
    outb(ata + ATA_REG_LBA1, (lba >> 8) & 0xFF);
    outb(ata + ATA_REG_LBA2, (lba >> 16) & 0xFF);
}

/* ~400ns settle: the status register is not valid for the first few reads
 * after a command byte is written, so poll it a few times before trusting it. */
static inline void ata_io_delay(u16 ata)
{
    for (int i = 0; i < 4; i++)
        (void)inb(ata + ATA_REG_STATUS);
}

/* Wait for a PIO data block to become available: BSY clears, no ERR/DF, DRQ set. */
static int ata_poll_data(u16 ata)
{
    for (int i = 0; i < 1000000; i++)
    {
        u8 st = inb(ata + ATA_REG_STATUS);
        if (st & ATA_SR_BSY)
        {
            io_wait();
            continue;
        }
        if (st & (ATA_SR_ERR | 0x20 /* DF */)) return -1;
        if (st & ATA_SR_DRQ) return 0;
        io_wait();
    }
    return -1;
}

static int block_read(struct block_dev *dev, u64 sector, void *buffer, size_t count)
{
    struct ata_device *ata_dev = (struct ata_device *)dev->private_data;
    u16 ata = ata_dev->io_base;
    u8 *buf = buffer;

    for (u64 s = sector; s < sector + count; s++, buf += ata_dev->sector_size)
    {
        if (ata_wait(ata, ATA_SR_BSY, 0) != 0) return -1;

        ata_setup_lba(ata, s);
        outb(ata + ATA_REG_COMMAND, ATA_CMD_READ_SECT);

        ata_io_delay(ata);
        if (ata_poll_data(ata) != 0) return -1;

        for (int i = 0; i < 256; ++i)
            ((u16 *)buf)[i] = inw(ata + ATA_REG_DATA);

        ata_io_delay(ata); /* let DRQ/status settle before the next command */
    }

    return 0;
}

static int block_write(struct block_dev *dev, u64 sector, void *buffer, size_t count)
{
    struct ata_device *ata_dev = (struct ata_device *)dev->private_data;
    u16 ata = ata_dev->io_base;
    const u8 *buf = buffer;

    for (u64 s = sector; s < sector + count; s++, buf += ata_dev->sector_size)
    {
        if (ata_wait(ata, ATA_SR_BSY, 0) != 0) return -1;

        ata_setup_lba(ata, s);
        outb(ata + ATA_REG_COMMAND, ATA_CMD_WRITE_SECT);

        ata_io_delay(ata);
        if (ata_poll_data(ata) != 0) return -1;

        for (int i = 0; i < 256; ++i)
            outw(ata + ATA_REG_DATA, ((const u16 *)buf)[i]);

        /* Let the drive commit the sector before the next command. */
        ata_io_delay(ata);
        if (ata_wait(ata, ATA_SR_BSY, 0) != 0) return -1;
    }

    outb(ata + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    if (ata_wait(ata, ATA_SR_BSY, 0) != 0) return -1;

    return 0;
}

static int block_flush(struct block_dev *dev)
{
    (void)dev;
    return 0;
}

// static inline void ata_general_isr(struct ata_device *dev)
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