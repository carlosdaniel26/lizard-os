#ifndef FAT16_H
#define FAT16_H

#include <kernel/drivers/ata.h>
#include <stdint.h>

#define FAT_NAME_LENGTH      8
#define FAT_EXT_LENGTH       3
#define FAT_DIR_ENTRY_SIZE   32
#define FAT_PADDING_CHAR     ' '
#define FAT16_EOC_MIN        0xFFF8
#define FAT16_CLUSTER_MIN    0x0002

/** BIOS Parameter Block (BPB) - Boot Sector */
typedef struct __attribute__((packed)) FAT16BPB
{
	uint8_t  jmp_boot[3];            /**   0: Jump instruction to boot code */
	uint8_t  oem_name[8];            /**   3: OEM Name identifier */
	uint16_t bytes_per_sector;       /**  11: Bytes per sector (usually 512) */
	uint8_t  sectors_per_cluster;    /**  13: Sectors per cluster */
	uint16_t reserved_sectors;       /**  14: Reserved sectors (usually 1) */
	uint8_t  num_fats;               /**  16: Number of FAT tables (usually 2) */
	uint16_t root_entry_count;       /**  17: Max entries in root directory */
	uint16_t total_sectors_16;       /**  19: Total sectors (if < 65536) */
	uint8_t  media;                  /**  21: Media descriptor */
	uint16_t sectors_per_fat;        /**  22: Sectors per FAT */
	uint16_t sectors_per_track;      /**  24: Sectors per track (CHS geometry) */
	uint16_t num_heads;              /**  26: Number of heads (CHS geometry) */
	uint32_t hidden_sectors;         /**  28: Hidden sectors (before partition) */
	uint32_t total_sectors_32;       /**  32: Total sectors (if total_sectors_16 == 0) */
} FAT16BPB;

typedef struct __attribute__((packed)) Fat16DirAttr {
	uint8_t read_only   : 1;
	uint8_t hidden      : 1; 
	uint8_t system		: 1;
	uint8_t volume_id	: 1;
	uint8_t directory	: 1;
	uint8_t archive		: 1;
	uint8_t reserved	: 2;
} Fat16DirAttr;

/** FAT16 Directory Entry (32 bytes) */
typedef struct __attribute__((packed)) FAT16Dir
{
	char     name[8];                /**  00: Filename (8 chars, padded with spaces) */
	char     ext[3];                 /**  08: Extension (3 chars, padded with spaces) */
	Fat16DirAttr  attributes;        /**  11: File attributes */
	uint8_t  reserved;               /**  12: Reserved */
	uint8_t  creation_time_tenths;   /**  13: Creation time (tenths of a second) */
	uint16_t creation_time;          /**  14: Creation time */
	uint16_t creation_date;          /**  16: Creation date */
	uint16_t last_access_date;       /**  18: Last access date */
	uint16_t first_cluster_high;     /**  20: High word of first cluster (FAT32 only) */
	uint16_t write_time;             /**  22: Last write time */
	uint16_t write_date;             /**  24: Last write date */
	uint16_t first_cluster_low;      /**  26: Low word of first cluster */
	uint32_t file_size;              /**  28: File size in bytes */
} FAT16Dir;

static int fat16_cmp_names(const char *fat_name, const char *name);
static int fat16_read_boot_sector(ATADevice *dev, FAT16BPB *bpb);
static char *fat16_read_root_directory(ATADevice *dev, const FAT16BPB *bpb);
static const FAT16Dir *fat16_find_file_entry(const char *filename, 
												 const char *root_dir, 
												 uint32_t entry_count);
static uint16_t *fat16_read_fat(ATADevice *dev, const FAT16BPB *bpb);
static void fat16_print_file_data(ATADevice *dev, const FAT16BPB *bpb, 
								 uint16_t start_cluster, uint32_t file_size,
								 const uint16_t *fat);
void fat16_test_read_bpb_and_file();

#endif