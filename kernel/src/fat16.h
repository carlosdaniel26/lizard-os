#ifndef FAT16_H
#define FAT16_H

#include <stddef.h>
#include <stdint.h>

#include <ata.h>

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
typedef struct FatHeader {
    uint8_t jump_boot[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sector_count;
    uint8_t num_fats;
    uint16_t root_entry_count; /* can't change*/
    uint16_t total_sectors_16;
    uint8_t media_descriptor;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
} __attribute__((packed)) FatHeader;

typedef struct Fat16Directory {
    uint8_t name[8];
    uint8_t extension[3];
    uint8_t attributes;
    uint8_t reserved1;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t reserved2;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t first_cluster_low;
    uint32_t file_size_bytes;
} __attribute__((packed)) Fat16Directory;

typedef struct Fat16 {
    FatHeader header;
    ATADevice *disk;

    uint32_t fat_start_lba;
    uint32_t root_dir_lba;
    uint32_t data_region_lba;

    uint32_t root_dir_sectors;
    uint32_t total_clusters;
} Fat16;

int fat16_mount(ATADevice *disk, Fat16 *fs);
#endif
