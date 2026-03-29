#include <ata.h>
#include <blk_dev.h>
#include <blkdev_manager.h>
#include <debug.h>
#include <fat16.h>
#include <fs.h>
#include <helpers.h>
#include <kmalloc.h>
#include <setup.h>
#include <stdio.h>
#include <string.h>
#include <vfs.h>

/* ===== CONSTANTS ===== */

/* FAT16 Boot Sector Signatures */

#define FAT16_BOOT_SIGNATURE 0x29
/* Sizes */
#define FAT16_FAT_ENTRY_SIZE 2
#define FAT16_DIR_ENTRY_SIZE 32
#define FILE_NAME_SIZE 11

/* Special directory entry values */
#define FAT16_DIR_ENTRY_DELETED 0xE5
#define FAT16_DIR_ENTRY_END 0x00

/* Cluster values */
#define FAT16_FREE_CLUSTER 0x0000
#define FAT16_RESERVED_CLUSTER 0x0001
#define FAT16_BAD_CLUSTER 0xFFF7
#define FAT16_EOC 0xFFF8

#define DIV_ROUND_UP(x, y) (((x) + (y) - 1) / (y))

static const char name[] = "fat16";

/* Forward declarations */
static int fat16_lookup(struct inode *dir, struct dentry *dentry);
static ssize_t fat16_read(struct file *file, char *buf, size_t count, off_t offset);
static int fat16_readdir(struct file *file, void *dirent, int (*filldir)(void *, const char *, int, off_t, u64));

static struct inode_ops fat16_inode_ops = {
    .lookup = fat16_lookup,
};

static struct file_ops fat16_file_ops = {
    .read = fat16_read,
    .readdir = fat16_readdir,
};

/* ===== INTERNAL HELPERS ===== */

static inline bool is_valid_entry(const struct fat16_directory *entry)
{
    if (entry->name[0] == FAT16_DIR_ENTRY_DELETED || entry->name[0] == FAT16_DIR_ENTRY_END)
    {
        return false;
    }

    if (entry->attributes == 0x0F) /* Long struct file Name entry */
        return false;

    /* Check ASCII characters */
    for (int i = 0; i < 11; i++)
    {
        if (entry->name[i] < 32 || entry->name[i] > 126) return false;
    }

    return true;
}

static u32 cluster_to_lba(struct fat16 *fs, u16 cluster)
{
    if (cluster < 2) return 0;
    return ((u32)(cluster - 2) * fs->header.sectors_per_cluster) + fs->data_region_lba;
}

static int read_fat_entry(struct fat16 *fs, u16 cluster, u16 *next)
{
    u32 fat_offset = (u32)cluster * FAT16_FAT_ENTRY_SIZE;
    u32 fat_sector = fs->fat_start_lba + (fat_offset / fs->header.bytes_per_sector);
    u32 entry_offset = fat_offset % fs->header.bytes_per_sector;

    char sector[512];
    if (blk_dev_read(fs->dev, fat_sector, sector, 1) != 0) return -1;

    memcpy(next, sector + entry_offset, sizeof(u16));
    return 0;
}

static u16 get_next_cluster(struct fat16 *fs, u16 cluster)
{
    u16 next = FAT16_EOC;
    if (read_fat_entry(fs, cluster, &next) != 0) return FAT16_EOC;
    return next;
}

static void convert_83_to_string(const u8 name[8], const u8 ext[3], char *out_string)
{
    int pos = 0;

    for (int i = 0; i < 8; i++)
    {
        if (name[i] != ' ') out_string[pos++] = name[i];
    }

    if (ext[0] != ' ')
    {
        out_string[pos++] = '.';

        for (int i = 0; i < 3; i++)
        {
            if (ext[i] != ' ') out_string[pos++] = ext[i];
        }
    }

    out_string[pos] = '\0';
}

static int compare_filenames(const char *filename, const struct fat16_directory *entry)
{
    char entry_name[13];
    convert_83_to_string(entry->name, entry->extension, entry_name);

    /* Case-insensitive comparison could be better, but let's stick to simple for now */
    return strcasecmp(filename, entry_name);
}

int fat16_detect(struct block_dev *dev)
{
    if (!dev || !dev->present) return -1;

    char boot_sector[512];
    if (dev->ops->read(dev, 0, boot_sector, 1) != 0) return -1;

    struct fat_header *header = (struct fat_header *)boot_sector;

    if (header->boot_signature == FAT16_BOOT_SIGNATURE) return 1;

    return -1;
}

int fat16_mount(struct block_dev *dev, struct fat16 *fs)
{
    if (!dev || !fs) return -1;

    fs->dev = dev;
    char buffer[512] = {0};
    if (dev->ops->read(dev, 0, buffer, 1) != 0)
    {
        debug_printf("ERROR reading boot sector\n");
        return -1;
    }
    memcpy(&fs->header, buffer, sizeof(struct fat_header));

    /* Calculate LBA values */
    fs->fat_start_lba = fs->header.reserved_sector_count;
    fs->root_dir_lba = fs->fat_start_lba + (fs->header.num_fats * fs->header.fat_size_16);
    fs->root_dir_sectors = (fs->header.root_entry_count * FAT16_DIR_ENTRY_SIZE + fs->header.bytes_per_sector - 1) / fs->header.bytes_per_sector;
    fs->data_region_lba = fs->root_dir_lba + fs->root_dir_sectors;

    return 0;
}

static int fat16_lookup(struct inode *dir, struct dentry *dentry)
{
    struct fat16 *fs = (struct fat16 *)dir->sb->fs_info;
    struct fat16_directory *dir_entry = (struct fat16_directory *)dir->private_data;
    char buffer[512];

    if (dir_entry->first_cluster_low == 0) /* root */
    {
        for (u32 sector = 0; sector < fs->root_dir_sectors; sector++)
        {
            if (blk_dev_read(fs->dev, fs->root_dir_lba + sector, buffer, 1) != 0) return -1;

            for (u16 i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++)
            {
                struct fat16_directory *entry = (struct fat16_directory *)(buffer + (i * FAT16_DIR_ENTRY_SIZE));

                if (entry->name[0] == FAT16_DIR_ENTRY_END) return -1;
                if (!is_valid_entry(entry)) continue;

                if (compare_filenames(dentry->name, entry) == 0)
                {
                    struct inode *inode = inode_alloc(dir->sb);
                    if (!inode) return -1;

                    struct fat16_directory *p = (struct fat16_directory *)zalloc(sizeof(struct fat16_directory));
                    memcpy(p, entry, sizeof(struct fat16_directory));

                    inode->private_data = p;
                    inode->size = entry->file_size_bytes;
                    inode->mode = (entry->attributes & FAT16_ATTR_DIRECTORY) ? 040777 : 0100666;
                    inode->i_ops = &fat16_inode_ops;
                    inode->f_ops = &fat16_file_ops;

                    dentry->inode = inode;
                    return 0;
                }
            }
        }
    }
    else
    {
        u16 current_cluster = dir_entry->first_cluster_low;
        do
        {
            u32 lba = cluster_to_lba(fs, current_cluster);
            for (u8 sector = 0; sector < fs->header.sectors_per_cluster; sector++)
            {
                if (blk_dev_read(fs->dev, lba + sector, buffer, 1) != 0) return -1;

                for (u16 i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++)
                {
                    struct fat16_directory *entry = (struct fat16_directory *)(buffer + (i * FAT16_DIR_ENTRY_SIZE));

                    if (entry->name[0] == FAT16_DIR_ENTRY_END) return -1;
                    if (!is_valid_entry(entry)) continue;

                    if (compare_filenames(dentry->name, entry) == 0)
                    {
                        struct inode *inode = inode_alloc(dir->sb);
                        if (!inode) return -1;

                        struct fat16_directory *p = (struct fat16_directory *)zalloc(sizeof(struct fat16_directory));
                        memcpy(p, entry, sizeof(struct fat16_directory));

                        inode->private_data = p;
                        inode->size = entry->file_size_bytes;
                        inode->mode = (entry->attributes & FAT16_ATTR_DIRECTORY) ? 040777 : 0100666;
                        inode->i_ops = &fat16_inode_ops;
                        inode->f_ops = &fat16_file_ops;

                        dentry->inode = inode;
                        return 0;
                    }
                }
            }
            current_cluster = get_next_cluster(fs, current_cluster);
        } while (current_cluster >= 0x0002 && current_cluster <= 0xFFEF);
    }

    return -1;
}

static ssize_t fat16_read(struct file *file, char *buf, size_t count, off_t offset)
{
    struct inode *inode = file->inode;
    struct fat16 *fs = (struct fat16 *)inode->sb->fs_info;
    struct fat16_directory *entry = (struct fat16_directory *)inode->private_data;

    if (offset >= (off_t)inode->size) return 0;
    if (offset + (off_t)count > (off_t)inode->size) count = inode->size - offset;

    u16 current_cluster = entry->first_cluster_low;
    u32 bytes_to_skip = (u32)offset;
    u32 bytes_read_total = 0;

    char sector_buffer[512];
    u32 sector_size = fs->header.bytes_per_sector;
    u32 sectors_per_cluster = fs->header.sectors_per_cluster;

    /* Skip clusters */
    while (bytes_to_skip >= sectors_per_cluster * sector_size)
    {
        bytes_to_skip -= sectors_per_cluster * sector_size;
        current_cluster = get_next_cluster(fs, current_cluster);
        if (current_cluster < 0x0002 || current_cluster > 0xFFEF) return 0;
    }

    while (current_cluster >= 0x0002 && current_cluster <= 0xFFEF && bytes_read_total < count)
    {
        u32 cluster_lba = cluster_to_lba(fs, current_cluster);

        for (u32 i = 0; i < sectors_per_cluster && bytes_read_total < count; i++)
        {
            if (bytes_to_skip >= sector_size)
            {
                bytes_to_skip -= sector_size;
                continue;
            }

            if (blk_dev_read(fs->dev, cluster_lba + i, sector_buffer, 1) != 0)
            {
                return bytes_read_total > 0 ? (ssize_t)bytes_read_total : -1;
            }

            u32 offset_in_sector = bytes_to_skip;
            u32 available = sector_size - offset_in_sector;
            u32 to_copy = (available < (count - bytes_read_total)) ? available : (u32)(count - bytes_read_total);

            memcpy(buf + bytes_read_total, sector_buffer + offset_in_sector, to_copy);
            bytes_read_total += to_copy;
            bytes_to_skip = 0;
        }

        current_cluster = get_next_cluster(fs, current_cluster);
    }

    return (ssize_t)bytes_read_total;
}

static int fat16_readdir(struct file *file, void *dirent, int (*filldir)(void *, const char *, int, off_t, u64))
{
    struct inode *inode = file->inode;
    struct fat16 *fs = (struct fat16 *)inode->sb->fs_info;
    struct fat16_directory *dir_entry = (struct fat16_directory *)inode->private_data;

    u32 offset = (u32)file->offset;
    int count = 0;
    char buffer[512];

    if (dir_entry->first_cluster_low == 0) /* root */
    {
        u32 start_entry = offset / FAT16_DIR_ENTRY_SIZE;
        for (u32 i = start_entry; i < fs->header.root_entry_count; i++)
        {
            u32 sector = i * FAT16_DIR_ENTRY_SIZE / fs->header.bytes_per_sector;
            u32 entry_in_sector = (i * FAT16_DIR_ENTRY_SIZE) % fs->header.bytes_per_sector;

            if (blk_dev_read(fs->dev, fs->root_dir_lba + sector, buffer, 1) != 0) break;

            struct fat16_directory *entry = (struct fat16_directory *)(buffer + entry_in_sector);

            if (entry->name[0] == FAT16_DIR_ENTRY_END) break;
            if (!is_valid_entry(entry))
            {
                offset += FAT16_DIR_ENTRY_SIZE;
                continue;
            }

            char name_str[13];
            convert_83_to_string(entry->name, entry->extension, name_str);

            if (filldir(dirent, name_str, strlen(name_str), offset, i) < 0) break;

            offset += FAT16_DIR_ENTRY_SIZE;
            count++;
        }
    }
    else
    {
        u16 current_cluster = dir_entry->first_cluster_low;
        u32 current_offset = 0;
        
        /* Skip clusters based on offset */
        while (offset >= current_offset + fs->header.sectors_per_cluster * fs->header.bytes_per_sector) {
            current_offset += fs->header.sectors_per_cluster * fs->header.bytes_per_sector;
            current_cluster = get_next_cluster(fs, current_cluster);
            if (current_cluster < 0x0002 || current_cluster > 0xFFEF) goto out;
        }

        while (current_cluster >= 0x0002 && current_cluster <= 0xFFEF)
        {
            u32 lba = cluster_to_lba(fs, current_cluster);
            for (u8 sector = 0; sector < fs->header.sectors_per_cluster; sector++)
            {
                if (blk_dev_read(fs->dev, lba + sector, buffer, 1) != 0) goto out;

                for (u16 i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++)
                {
                    if (current_offset < offset) {
                        current_offset += FAT16_DIR_ENTRY_SIZE;
                        continue;
                    }

                    struct fat16_directory *entry = (struct fat16_directory *)(buffer + (i * FAT16_DIR_ENTRY_SIZE));
                    if (entry->name[0] == FAT16_DIR_ENTRY_END) goto out;
                    if (!is_valid_entry(entry))
                    {
                        offset += FAT16_DIR_ENTRY_SIZE;
                        current_offset += FAT16_DIR_ENTRY_SIZE;
                        continue;
                    }

                    char name_str[13];
                    convert_83_to_string(entry->name, entry->extension, name_str);

                    if (filldir(dirent, name_str, strlen(name_str), offset, 0) < 0) goto out;

                    offset += FAT16_DIR_ENTRY_SIZE;
                    current_offset += FAT16_DIR_ENTRY_SIZE;
                    count++;
                }
            }
            current_cluster = get_next_cluster(fs, current_cluster);
        }
    }

out:
    file->offset = offset;
    return count;
}

struct dentry *fat16_mount_fs(struct super_block *sb, const void *data)
{
    struct block_dev *dev = (struct block_dev *)data;
    struct fat16 *fs = (struct fat16 *)zalloc(sizeof(struct fat16));
    if (!fs) return NULL;

    if (fat16_mount(dev, fs) != 0)
    {
        kfree(fs);
        return NULL;
    }

    sb->fs_info = fs;

    struct inode *root_inode = inode_alloc(sb);
    if (!root_inode)
    {
        kfree(fs);
        return NULL;
    }

    struct fat16_directory *root_entry = (struct fat16_directory *)zalloc(sizeof(struct fat16_directory));
    root_entry->attributes = FAT16_ATTR_DIRECTORY;
    root_entry->first_cluster_low = 0;

    root_inode->private_data = root_entry;
    root_inode->mode = 040777; /* Directory */
    root_inode->i_ops = &fat16_inode_ops;
    root_inode->f_ops = &fat16_file_ops;

    struct dentry *root_dentry = dentry_alloc("/");
    root_dentry->inode = root_inode;
    root_dentry->parent = root_dentry;
    sb->root = root_dentry;

    return root_dentry;
}

int fat16_init()
{
    struct fs_type *type = (struct fs_type *)zalloc(sizeof(struct fs_type));
    if (!type)
    {
        return -1;
    }

    strcpy(type->name, name);
    type->mount = fat16_mount_fs;

    return fs_register(type);
}

fs_initcall(fat16_init);
