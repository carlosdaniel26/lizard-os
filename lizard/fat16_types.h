#pragma once

#include <nolibc/types.h>

#include <lizard/blk_dev.h>

/*
 * On-disk FAT16 layout.
 *
 * Volume map: [reserved sectors][FATs][root directory][data region]
 *
 * jump_boot: first 3 bytes of sector 0; on a real boot volume the BIOS jumps
 *            here. oem_name: Original Equipment Manufacturer string.
 */

#define FAT16_SECTOR_SIZE    512
#define FAT16_FAT_ENTRY_SIZE 2
#define FAT16_DIR_ENTRY_SIZE 32
#define FAT16_NAME_MAX       13 /* 8 + '.' + 3 + '\0' */

/* Extended boot signature, BPB offset 38. */
#define FAT16_BOOT_SIGNATURE 0x29

/* Directory entry name[0] sentinels. */
#define FAT16_DIR_ENTRY_DELETED 0xE5
#define FAT16_DIR_ENTRY_END     0x00

/* Directory entry attributes. */
#define FAT16_ATTR_READ_ONLY 0x01
#define FAT16_ATTR_HIDDEN    0x02
#define FAT16_ATTR_SYSTEM    0x04
#define FAT16_ATTR_VOLUME_ID 0x08
#define FAT16_ATTR_DIRECTORY 0x10
#define FAT16_ATTR_ARCHIVE   0x20
#define FAT16_ATTR_LFN       0x0F /* attributes == this: long-file-name slot */

/* FAT cluster values. */
#define FAT16_FREE_CLUSTER     0x0000
#define FAT16_RESERVED_CLUSTER 0x0001
#define FAT16_CLUSTER_MIN      0x0002
#define FAT16_CLUSTER_MAX      0xFFEF
#define FAT16_BAD_CLUSTER      0xFFF7
#define FAT16_EOC              0xFFF8

struct fat_header {
    u8 jump_boot[3];
    u8 oem_name[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sector_count;
    u8 num_fats;
    u16 root_entry_count;
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
} __attribute__((packed));

struct fat16_directory {
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
} __attribute__((packed));

/* Mounted-volume runtime state (BPB plus derived LBAs). */
struct fat16 {
    struct fat_header header;
    struct block_dev *dev;

    u32 fat_start_lba;
    u32 root_dir_lba;
    u32 data_region_lba;

    u32 root_dir_sectors;
    u32 total_clusters;
};
