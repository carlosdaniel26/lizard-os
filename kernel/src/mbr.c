#include <blk_dev.h>
#include <blkdev_manager.h>
#include <debug.h>
#include <mbr.h>
#include <stdio.h>

static int mbr_read(BlockDevice *dev, MbrHeader *mbr)
{
    /* Read first sector (LBA 0) */
    return blk_dev_read(dev, 0, mbr, 1);
}

/* Write MBR to disk */
static int mbr_write(BlockDevice *dev, const MbrHeader *mbr)
{
    /* Write first sector (LBA 0) */
    return blk_dev_write(dev, 0, mbr, 1);
}

int mbr_scan(BlockDevice *dev)
{
    debug_printf("starting mbr_scan..\n");
    MbrHeader mbr;

    if (mbr_read(dev, &mbr) == -1)
    {
        return -1;
    }

    if (!mbr_is_valid(&mbr))
    {
        return 0;
    }

    int created = 0;

    for (int i = 0; i < MBR_PARTITION_COUNT; i++)
    {
        const MbrPartitionEntry *partition = &mbr.partitions[i];

        if (!mbr_partition_is_used(partition)) continue;

        if (partition->type == MBRYPE_GPT_PROTECTIVE) return 0; /* done here, let GPT handle that */

        u64 start = partition->lba_first;
        u64 count = partition->sector_count;

        if (count == 0) continue;

        if (start + count > dev->total_sectors) continue;

        kprintf("mbr: partition %u type=0x%x start=%u sectors=%u\n", i + 1, partition->type, start, count);

        if (blkdev_create_partition(dev, i, start, count)) created++;
    }

    return created;
}
