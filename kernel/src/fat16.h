#pragma once

#include <stddef.h>
#include <types.h>

#include <blk_dev.h>

/* Map: [Reserved sector][FATs][Root directory][Data region] */

/* BIOS Parameter Block (BPB) for FAT16
 *
 *
 * jump_boot: its the first sector of a disk, which can also contains initial
 * code which bios can load, the BIOS uses the first 3 bytes (jump_boot)
 * as an address to boot.
 *
 * oem_name: Original Equipment Manufacturer
 *
 *
 *
 */

/* Attributes */
#define FAT16_ATTR_READ_ONLY 0x01
#define FAT16_ATTR_HIDDEN 0x02
#define FAT16_ATTR_SYSTEM 0x04
#define FAT16_ATTR_VOLUME_ID 0x08
#define FAT16_ATTR_DIRECTORY 0x10
#define FAT16_ATTR_ARCHIVE 0x20

typedef struct FatHeader {
    u8 jump_boot[3];
    u8 oem_name[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sector_count;
    u8 num_fats;
    u16 root_entry_count; /* can't change*/
    u16 total_sectors_16;
    u8 media_descriptor;
    u16 fat_size_16;
    u16 sectors_per_track;
    u16 num_heads;
    u32 hidden_sectors;
    u32 total_sectors_32;
    u8 drive_number;
    u8 reserved;
    u8 boot_signature;
    u32 volume_id;
    u8 volume_label[11];
    u8 fs_type[8];
} __attribute__((packed)) FatHeader;

typedef struct Fat16Directory {
    u8 name[8];
    u8 extension[3];
    u8 attributes;
    u8 reserved1;
    u8 creation_time_tenths;
    u16 creation_time;
    u16 creation_date;
    u16 last_access_date;
    u16 reserved2;
    u16 write_time;
    u16 write_date;
    u16 first_cluster_low;
    u32 file_size_bytes;
} __attribute__((packed)) Fat16Directory;

typedef struct Fat16 {
    FatHeader header;
    BlockDevice *dev;

    u32 fat_start_lba;
    u32 root_dir_lba;
    u32 data_region_lba;

    u32 root_dir_sectors;
    u32 total_clusters;
} Fat16;

int fat16_detect(BlockDevice *dev);
int fat16_mount(BlockDevice *dev, Fat16 *fs);
void test_fat16(void);
void fat16_init(void);
