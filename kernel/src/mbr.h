#pragma once

#include <blk_dev.h>
#include <stdbool.h>
#include <types.h>

/* MBR layout constants */

#define MBR_SECTOR_SIZE 512
#define MBR_BOOT_CODE_SIZE 446
#define MBR_PARTITION_COUNT 4
#define MBR_PARTITION_ENTRY_SIZE 16
#define MBR_PARTITION_TABLE_OFFSET 446
#define MBR_SIGNATURE_OFFSET 510
#define MBR_SIGNATURE 0xAA55

/* Partition status flags */

#define MBR_PARTITION_INACTIVE 0x00
#define MBR_PARTITION_ACTIVE 0x80

/* Partition types */

#define MBRYPE_EMPTY 0x00

#define MBRYPE_FAT12 0x01
#define MBRYPE_FAT16_SMALL 0x04
#define MBRYPE_EXTENDED_CHS 0x05
#define MBRYPE_FAT16 0x06
#define MBRYPE_NTFS_EXFAT 0x07

#define MBRYPE_FAT32_CHS 0x0B
#define MBRYPE_FAT32_LBA 0x0C
#define MBRYPE_FAT16_LBA 0x0E
#define MBRYPE_EXTENDED_LBA 0x0F

#define MBRYPE_LINUX 0x83
#define MBRYPE_LINUX_EXTENDED 0x85
#define MBRYPE_LINUX_SWAP 0x82

#define MBRYPE_GPT_PROTECTIVE 0xEE

/* CHS helpers (legacy, usually unused) */

#define MBR_CHS_SECTOR_MASK 0x3F
#define MBR_CHS_CYLINDER_HIGH_MASK 0xC0

typedef struct __attribute__((packed)) {
    u8 status;
    u8 chs_first[3];
    u8 type;
    u8 chs_last[3];
    u32 lba_first;
    u32 sector_count;
} MbrPartitionEntry;

typedef struct __attribute__((packed)) {
    u8 boot_code[MBR_BOOT_CODE_SIZE];
    MbrPartitionEntry partitions[MBR_PARTITION_COUNT];
    u16 signature;
} MbrHeader;

typedef struct {
    BlockDevice *parent_dev;
    u64 lba_start;
    u64 sector_count;
} MbrPartitionContext;

typedef struct {
    u64 start_lba;
    u64 sector_count;
} MBRPartitionData;

/* Helpers */

static inline bool mbr_is_valid(const MbrHeader *mbr)
{
    return mbr && mbr->signature == MBR_SIGNATURE;
}

static inline bool mbr_partition_is_used(const MbrPartitionEntry *p)
{
    return p && p->type != MBRYPE_EMPTY && p->sector_count != 0;
}

static inline bool mbr_partition_is_active(const MbrPartitionEntry *p)
{
    return p && p->status == MBR_PARTITION_ACTIVE;
}

int mbr_scan(BlockDevice *dev);
