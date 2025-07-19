#include <ata.h>
#include <fat16.h>
#include <helpers.h>
#include <stdio.h>
#include <string.h>
#include <kmalloc.h>

/* Map: [Reserved][FATs][Root Dir][Data] */

#define FAT16_FAT_ENTRY_SIZE 2
#define FAT16_DIR_ENTRY_SIZE 32
#define FILE_NAME_SIZE 11

#define FAT16_DIR_ENTRY_DELETED 0xE5
#define FAT16_DIR_ENTRY_END 0x00

#define FAT16_ATTR_READ_ONLY 0x01
#define FAT16_ATTR_HIDDEN 0x02
#define FAT16_ATTR_SYSTEM 0x04
#define FAT16_ATTR_VOLUME_ID 0x08
#define FAT16_ATTR_DIRECTORY 0x10
#define FAT16_ATTR_ARCHIVE 0x20

#define DIV_ROUND_UP(x, y) (((x) + (y)-1) / (y))

/* === Internal helpers === */

static inline int get_sector_size(Fat16 *fs)
{
    return fs->bpb.bytes_per_sector ? fs->bpb.bytes_per_sector : 512;
}

static uint32_t fat16_cluster_to_lba(Fat16 *fs, uint16_t cluster)
{
    /*
     * First 2 values for cluster nums (0x0 and 0x1) are not available.
     * The place for them in FAT table is used to store the FAT signature.
     * First cluster number is 0x2.
     *
     * It is, the clusters 0x0 and 0x1 will never be called, but when a
     * cluster >= 2 comes in, it will have to come 2 steps back to
     * data_region base.
     */
    return fs->data_region_lba + ((cluster - 2) * fs->bpb.sectors_per_cluster);
}

static uint16_t fat16_next_cluster(Fat16 *fs, uint16_t cluster)
{
    uint32_t fat_offset = cluster * FAT16_FAT_ENTRY_SIZE;
    uint32_t fat_sector = fs->fat_start_lba + (fat_offset / fs->bpb.bytes_per_sector);
    uint32_t ent_offset = fat_offset % fs->bpb.bytes_per_sector;

    /* Check on the Table which is the next cluster on the chain */
    char sector[512];
    if (atapio_read_sector(fs->disk, fat_sector, sector) != 0)
        return 0xFFF8;

    uint16_t next;
    memcpy(&next, sector + ent_offset, sizeof(uint16_t));
    return next;
}

static int fat16_compare_filename(const char *filename, const Fat16Directory *entry)
{
    char formatted[FILE_NAME_SIZE];
    memset(formatted, ' ', sizeof(formatted));

    const char *dot = strchr(filename, '.');
    size_t name_len = dot ? (size_t)(dot - filename) : strlen(filename);
    if (name_len > 8)
        name_len = 8;
    memcpy(formatted, filename, name_len);

    if (dot)
    {
        size_t ext_len = strlen(dot + 1);
        if (ext_len > 3)
            ext_len = 3;
        memcpy(formatted + 8, dot + 1, ext_len);
    }

    for (int i = 0; i < FILE_NAME_SIZE; i++)
        formatted[i] = toupper((unsigned char)formatted[i]);

    return memcmp(formatted, entry->name, FILE_NAME_SIZE) == 0;
}

static int _fat16_read_dir_common(Fat16 *fs, uint32_t start_lba, uint32_t total_sectors, Fat16Directory *out, const char *filename)
{
    char buffer[512];
    int sector_size = get_sector_size(fs);

    for (uint32_t i = 0; i < total_sectors; i++)
    {
        if (atapio_read_sector(fs->disk, start_lba + i, buffer) != 0)
            return -1;

        for (int j = 0; j < sector_size / FAT16_DIR_ENTRY_SIZE; j++)
        {
            Fat16Directory *entry = (Fat16Directory *)(buffer + j * FAT16_DIR_ENTRY_SIZE);

            if (entry->name[0] == FAT16_DIR_ENTRY_END)
                return -1;

            if (entry->name[0] == FAT16_DIR_ENTRY_DELETED)
                continue;

            if (entry->attributes & FAT16_ATTR_VOLUME_ID)
                continue;

            if (fat16_compare_filename(filename, entry))
            {
                memcpy(out, entry, sizeof(Fat16Directory));
                return 0;
            }
        }
    }

    return -1;
}

int fat16_read_root_dir(Fat16 *fs, const char *filename, Fat16Directory *out)
{
    return _fat16_read_dir_common(
        fs,
        fs->root_dir_lba,
        fs->root_dir_sectors,
        out,
        filename);
}

/* === Public API === */

int fat16_mount(Fat16 *fs, ATADevice *disk)
{
    if (!disk || !disk->present)
        return -1;

    char buffer[512];
    if (atapio_read_sector(disk, 0, buffer) != 0)
        return -1;

    fs->disk = disk;
    memcpy(&fs->bpb, buffer, sizeof(Fat16Bpb));

    fs->fat_start_lba = fs->bpb.reserved_sector_count;

    fs->root_dir_sectors = DIV_ROUND_UP(fs->bpb.root_entry_count * FAT16_DIR_ENTRY_SIZE, fs->bpb.bytes_per_sector);

    /* Root dir lba is right after the FAT */
    fs->root_dir_lba = fs->fat_start_lba + (fs->bpb.num_fats * fs->bpb.fat_size_16);

    /* Data region is right after root lba */
    fs->data_region_lba = fs->root_dir_lba + fs->root_dir_sectors;

    uint32_t total_sectors = fs->bpb.total_sectors_16 ? fs->bpb.total_sectors_16 : fs->bpb.total_sectors_32;

    uint32_t non_data = fs->bpb.reserved_sector_count + (fs->bpb.num_fats * fs->bpb.fat_size_16) + fs->root_dir_sectors;

    fs->total_clusters = (total_sectors - non_data) / fs->bpb.sectors_per_cluster;

    return 0;
}

int fat16_find_in_dir(Fat16 *fs, Fat16Directory *dir, char *filename, Fat16Directory *out)
{
    if (!(dir->attributes & FAT16_ATTR_DIRECTORY))
        return -1;

    // Root directory is special case
    if (dir->first_cluster_low == 0)
        return fat16_read_root_dir(fs, filename, out);

    uint16_t cluster = dir->first_cluster_low;
    char buffer[512];

    while (cluster < 0xFFF8)
    {
        uint32_t lba = fat16_cluster_to_lba(fs, cluster);

        for (int i = 0; i < fs->bpb.sectors_per_cluster; i++)
        {
            if (atapio_read_sector(fs->disk, lba + i, buffer) != 0)
                return -1;

            for (int j = 0; j < get_sector_size(fs) / FAT16_DIR_ENTRY_SIZE; j++)
            {
                Fat16Directory *entry = (Fat16Directory *)(buffer + j * FAT16_DIR_ENTRY_SIZE);

                if (entry->name[0] == FAT16_DIR_ENTRY_END)
                    return -1;

                if (entry->name[0] == FAT16_DIR_ENTRY_DELETED)
                    continue;

                if (entry->attributes & FAT16_ATTR_VOLUME_ID)
                    continue;

                if (fat16_compare_filename(filename, entry))
                {
                    memcpy(out, entry, sizeof(Fat16Directory));
                    return 0;
                }
            }
        }

        cluster = fat16_next_cluster(fs, cluster);
    }

    return -1;
}

int fat16_read_dir(Fat16 *fs, Fat16Directory *entry, char *out_buffer)
{
    if (!fs || !entry || !out_buffer)
        return -1;

    uint16_t cluster = entry->first_cluster_low;
    if (cluster < 2)
        return -1;

    uint32_t sector_size = get_sector_size(fs);
    uint32_t bytes_read = 0;
    uint32_t file_size = entry->file_size_bytes;

    while (cluster < 0xFFF8 && bytes_read < file_size)
    {
        uint32_t lba = fat16_cluster_to_lba(fs, cluster);

        for (uint32_t i = 0; i < fs->bpb.sectors_per_cluster; i++)
        {
            if (bytes_read >= file_size)
                break;

            char sector[512];

            if (atapio_read_sector(fs->disk, lba + i, sector) != 0)
                return -1;

            uint32_t to_copy = sector_size;

            if ((bytes_read + to_copy) > file_size)
                to_copy = file_size - bytes_read;

            memcpy(out_buffer + bytes_read, sector, to_copy);
            bytes_read += to_copy;
        }

        cluster = fat16_next_cluster(fs, cluster);
    }
    return 0;
}

void read_and_print_file(Fat16 *fs, const char *filename)
{
    Fat16Directory dir = {0};
    Fat16Directory root = {0};
    root.attributes = FAT16_ATTR_DIRECTORY;
    root.first_cluster_low = 0; // Root dir has no cluster

    fat16_find_in_dir(fs, &root, (char *)filename, &dir);

    kprintf("file_size: %u\n", dir.file_size_bytes);

    char *buffer = kmalloc(dir.file_size_bytes);
    memset(buffer, 0, dir.file_size_bytes);
    
    if (fat16_read_dir(fs, &dir, buffer) != 0)
    {
        kprintf("Error reading file: %s\n", filename);
        return;
    }

    buffer[strlen(buffer) - 1] = '\0'; // Ensure null-termination

    kprintf("===== File: %s =====\n%s\n", filename, buffer);
}
