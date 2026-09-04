#include <lizard/blk_dev.h>
#include <lizard/debug.h>
#include <lizard/fat16.h>
#include <lizard/fat16_helpers.h>
#include <lizard/fs.h>
#include <lizard/init.h>
#include <lizard/kmalloc.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <lizard/vfs.h>

/* inode->mode for the two entry kinds we expose */
#define FAT16_MODE_DIR  040777
#define FAT16_MODE_FILE 0100666

static const char name[] = "fat16";

/*
 * Per-inode state: a copy of the on-disk directory entry plus where that entry
 * lives, so writes can persist size / first-cluster changes. For the root
 * directory `loc` is unused (loc.lba == 0).
 */
struct fat16_inode_info {
    struct fat16_directory ent;
    struct fat16_dir_loc loc;
};

static int fat16_lookup(struct inode *dir, struct dentry *dentry);
static int fat16_create(struct inode *dir, struct dentry *dentry, int mode);
static int fat16_mkdir(struct inode *dir, struct dentry *dentry, int mode);
static int fat16_unlink(struct inode *dir, struct dentry *dentry);
static int fat16_rmdir(struct inode *dir, struct dentry *dentry);
static int fat16_truncate(struct inode *inode, u64 length);

static int fat16_open(struct inode *inode, struct file *file);
static int fat16_release(struct inode *inode, struct file *file);
static ssize_t fat16_read(struct file *file, char *buf, size_t count, off_t offset);
static ssize_t fat16_write(struct file *file, const char *buf, size_t count, off_t offset);
static int fat16_readdir(struct file *file, void *dirent, int (*filldir)(void *, struct dirent *));

static struct inode_ops fat16_inode_ops = {
    .lookup = fat16_lookup,
    .create = fat16_create,
    .mkdir  = fat16_mkdir,
    .unlink = fat16_unlink,
    .rmdir  = fat16_rmdir,
    .truncate = fat16_truncate,
};

static struct file_ops fat16_file_ops = {
    .open    = fat16_open,
    .read    = fat16_read,
    .write   = fat16_write,
    .readdir = fat16_readdir,
    .release = fat16_release,
};

/* ===== helpers ===== */

static struct fat16 *fs_of(struct inode *inode)
{
    return inode->sb->fs_info;
}

static struct fat16_directory *ent_of(struct inode *inode)
{
    return &((struct fat16_inode_info *)inode->private_data)->ent;
}

static bool is_dir(const struct fat16_directory *ent)
{
    return ent->attributes & FAT16_ATTR_DIRECTORY;
}

static struct inode *fat16_make_inode(struct super_block *sb,
                                      const struct fat16_directory *ent,
                                      struct fat16_dir_loc loc)
{
    struct inode *inode = inode_alloc(sb);
    if (!inode) return NULL;

    struct fat16_inode_info *info = zalloc(sizeof(*info));
    if (!info)
    {
        inode_free(inode);
        return NULL;
    }

    info->ent = *ent;
    info->loc = loc;

    inode->private_data = info;
    inode->size = ent->file_size_bytes;
    inode->mode = is_dir(ent) ? FAT16_MODE_DIR : FAT16_MODE_FILE;
    inode->i_ops = &fat16_inode_ops;
    inode->f_ops = &fat16_file_ops;
    return inode;
}

/* Build a fresh, time-stamped 8.3 entry with the given name and attributes. */
static int fat16_fill_entry(struct fat16_directory *ent, const char *name_str, u8 attributes)
{
    memset(ent, 0, sizeof(*ent));
    if (!fat16_str_to_name(name_str, ent->name, ent->extension))
        return -1;
    ent->attributes = attributes;
    fat16_entry_stamp(ent);
    return 0;
}

/* ===== superblock / mount ===== */

int fat16_detect(struct block_dev *dev)
{
    if (!dev || !dev->present) return -1;

    char boot_sector[FAT16_SECTOR_SIZE];
    if (dev->ops->read(dev, 0, boot_sector, 1) != 0) return -1;

    const struct fat_header *header = (const struct fat_header *)boot_sector;

    return header->boot_signature == FAT16_BOOT_SIGNATURE ? 1 : -1;
}

int fat16_mount(struct block_dev *dev, struct fat16 *fs)
{
    if (!dev || !fs) return -1;

    char boot_sector[FAT16_SECTOR_SIZE];
    if (dev->ops->read(dev, 0, boot_sector, 1) != 0)
    {
        debug_printf("fat16: failed to read boot sector\n");
        return -1;
    }

    fs->dev = dev;
    memcpy(&fs->header, boot_sector, sizeof(fs->header));

    fs->fat_start_lba = fs->header.reserved_sector_count;
    fs->root_dir_lba = fs->fat_start_lba + (fs->header.num_fats * fs->header.fat_size_16);
    fs->root_dir_sectors =
        (fs->header.root_entry_count * FAT16_DIR_ENTRY_SIZE + fs->header.bytes_per_sector - 1) /
        fs->header.bytes_per_sector;
    fs->data_region_lba = fs->root_dir_lba + fs->root_dir_sectors;

    u32 total = fs->header.total_sectors_16 ? fs->header.total_sectors_16
                                            : fs->header.total_sectors_32;
    fs->total_clusters = (total - fs->data_region_lba) / fs->header.sectors_per_cluster;

    /* Pull the first FAT into RAM so chain walks are memory lookups. */
    fs->fat_bytes = (u32)fs->header.fat_size_16 * fs->header.bytes_per_sector;
    fs->fat_cache = kmalloc(fs->fat_bytes);
    if (fs->fat_cache)
    {
        for (u32 i = 0; i < fs->header.fat_size_16; i++)
        {
            if (blk_dev_read(fs->dev, fs->fat_start_lba + i,
                             fs->fat_cache + i * fs->header.bytes_per_sector, 1) != 0)
            {
                kfree(fs->fat_cache);
                fs->fat_cache = NULL;
                break;
            }
        }
    }

    return 0;
}

struct dentry *fat16_mount_fs(struct super_block *sb, const void *data)
{
    struct block_dev *dev = (struct block_dev *)data;

    struct fat16 *fs = zalloc(sizeof(*fs));
    if (!fs) return NULL;

    if (fat16_mount(dev, fs) != 0)
    {
        kfree(fs);
        return NULL;
    }
    sb->fs_info = fs;

    struct fat16_directory root_ent;
    memset(&root_ent, 0, sizeof(root_ent));
    root_ent.attributes = FAT16_ATTR_DIRECTORY;
    root_ent.first_cluster_low = 0;

    struct inode *root_inode = fat16_make_inode(sb, &root_ent, (struct fat16_dir_loc){0, 0});
    struct dentry *root_dentry = dentry_alloc("/");

    if (!root_inode || !root_dentry)
    {
        kfree(fs);
        return NULL;
    }

    root_dentry->inode = root_inode;
    root_dentry->parent = root_dentry;
    sb->root = root_dentry;

    return root_dentry;
}

/* ===== lookup / readdir ===== */

struct lookup_ctx {
    struct super_block *sb;
    const char *name;
    struct dentry *dentry;
};

static int lookup_cb(const struct fat16_directory *entry, u32 offset,
                     struct fat16_dir_loc loc, void *ctxp)
{
    struct lookup_ctx *ctx = ctxp;
    (void)offset;

    if (!fat16_entry_valid(entry)) return FAT16_WALK_CONTINUE;
    if (fat16_name_cmp(ctx->name, entry) != 0) return FAT16_WALK_CONTINUE;

    struct inode *inode = fat16_make_inode(ctx->sb, entry, loc);
    if (!inode) return -1;

    ctx->dentry->inode = inode;
    return FAT16_WALK_STOP;
}

static int fat16_lookup(struct inode *dir, struct dentry *dentry)
{
    struct lookup_ctx ctx = {
        .sb = dir->sb,
        .name = dentry->name,
        .dentry = dentry,
    };

    return fat16_walk_dir(fs_of(dir), ent_of(dir)->first_cluster_low, lookup_cb, &ctx) ==
                   FAT16_WALK_STOP
               ? 0
               : -1;
}

struct readdir_ctx {
    int (*filldir)(void *, struct dirent *);
    void *dirent;
    u32 start; /* file->offset: entries before this were already returned */
    u32 next;  /* where the next readdir() call should resume */
    int count;
};

static int readdir_cb(const struct fat16_directory *entry, u32 offset,
                      struct fat16_dir_loc loc, void *ctxp)
{
    struct readdir_ctx *ctx = ctxp;
    (void)loc;

    if (offset < ctx->start) return FAT16_WALK_CONTINUE;
    ctx->next = offset + FAT16_DIR_ENTRY_SIZE;

    if (!fat16_entry_valid(entry)) return FAT16_WALK_CONTINUE;

    char name_str[FAT16_NAME_MAX];
    fat16_name_to_str(entry->name, entry->extension, name_str);

    struct dirent d = {
        .inode = 0,
        .offset = offset,
        .reclen = sizeof(struct dirent),
        .type = is_dir(entry) ? DT_DIR : DT_REG,
    };
    strncpy(d.name, name_str, sizeof(d.name));

    if (ctx->filldir(ctx->dirent, &d) < 0) return FAT16_WALK_STOP;

    ctx->count++;
    return FAT16_WALK_CONTINUE;
}

static int fat16_readdir(struct file *file, void *dirent, int (*filldir)(void *, struct dirent *))
{
    struct readdir_ctx ctx = {
        .filldir = filldir,
        .dirent = dirent,
        .start = (u32)file->offset,
        .next = (u32)file->offset,
        .count = 0,
    };

    fat16_walk_dir(fs_of(file->inode), ent_of(file->inode)->first_cluster_low, readdir_cb, &ctx);

    file->offset = ctx.next;
    return ctx.count;
}

/* ===== file I/O ===== */

static int fat16_open(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    return 0;
}

static int fat16_release(struct inode *inode, struct file *file)
{
    (void)inode;
    (void)file;
    return 0;
}

static ssize_t fat16_read(struct file *file, char *buf, size_t count, off_t offset)
{
    struct inode *inode = file->inode;
    struct fat16 *fs = fs_of(inode);
    struct fat16_directory *entry = ent_of(inode);

    if (offset >= (off_t)inode->size) return 0;
    if (offset + (off_t)count > (off_t)inode->size) count = inode->size - offset;

    const u32 sector_size = fs->header.bytes_per_sector;
    const u32 cluster_size = sector_size * fs->header.sectors_per_cluster;

    u16 cluster = entry->first_cluster_low;
    u32 skip = (u32)offset;
    u32 done = 0;

    for (; skip >= cluster_size; skip -= cluster_size)
    {
        cluster = fat16_next_cluster(fs, cluster);
        if (!fat16_cluster_valid(cluster)) return 0;
    }

    char sector[FAT16_SECTOR_SIZE];

    while (fat16_cluster_valid(cluster) && done < count)
    {
        u32 lba = fat16_cluster_to_lba(fs, cluster);

        for (u32 i = 0; i < fs->header.sectors_per_cluster && done < count; i++)
        {
            if (skip >= sector_size)
            {
                skip -= sector_size;
                continue;
            }

            if (blk_dev_read(fs->dev, lba + i, sector, 1) != 0)
                return done > 0 ? (ssize_t)done : -1;

            u32 avail = sector_size - skip;
            u32 to_copy = avail < (count - done) ? avail : (u32)(count - done);

            memcpy(buf + done, sector + skip, to_copy);
            done += to_copy;
            skip = 0;
        }

        cluster = fat16_next_cluster(fs, cluster);
    }

    return (ssize_t)done;
}

/* Follow `cluster`'s chain by one link, allocating and linking a new cluster if
 * the chain ends. Returns the next cluster or 0 on failure. */
static u16 chain_step(struct fat16 *fs, u16 cluster)
{
    u16 next = fat16_next_cluster(fs, cluster);
    if (fat16_cluster_valid(next)) return next;

    if (fat16_cluster_alloc(fs, &next) != 0) return 0;
    if (fat16_fat_set(fs, cluster, next) != 0) return 0;
    return next;
}

static ssize_t fat16_write(struct file *file, const char *buf, size_t count, off_t offset)
{
    struct inode *inode = file->inode;
    struct fat16 *fs = fs_of(inode);
    struct fat16_inode_info *info = inode->private_data;
    struct fat16_directory *ent = &info->ent;

    if (is_dir(ent) || fs->dev->read_only) return -1;
    if (count == 0) return 0;

    const u32 sector_size = fs->header.bytes_per_sector;
    const u32 cluster_size = sector_size * fs->header.sectors_per_cluster;

    if (ent->first_cluster_low == 0)
    {
        u16 c;
        if (fat16_cluster_alloc(fs, &c) != 0) return -1;
        ent->first_cluster_low = c;
    }

    u16 cluster = ent->first_cluster_low;
    for (u32 pos = cluster_size; pos <= (u32)offset; pos += cluster_size)
    {
        cluster = chain_step(fs, cluster);
        if (!cluster) return -1;
    }

    u32 skip = (u32)offset % cluster_size;
    u32 done = 0;
    char sector[FAT16_SECTOR_SIZE];

    while (done < count)
    {
        u32 lba = fat16_cluster_to_lba(fs, cluster);

        for (u32 i = 0; i < fs->header.sectors_per_cluster && done < count; i++)
        {
            if (skip >= sector_size)
            {
                skip -= sector_size;
                continue;
            }

            u32 avail = sector_size - skip;
            u32 chunk = avail < (count - done) ? avail : (u32)(count - done);

            if (skip != 0 || chunk != sector_size)
            {
                if (blk_dev_read(fs->dev, lba + i, sector, 1) != 0) goto out;
            }
            memcpy(sector + skip, buf + done, chunk);
            if (blk_dev_write(fs->dev, lba + i, sector, 1) != 0) goto out;

            done += chunk;
            skip = 0;
        }

        if (done < count)
        {
            cluster = chain_step(fs, cluster);
            if (!cluster) goto out;
        }
    }

out:
    if ((u64)offset + done > inode->size)
    {
        inode->size = (u64)offset + done;
        ent->file_size_bytes = (u32)inode->size;
    }
    if (done > 0) fat16_entry_touch(ent);
    fat16_dir_write(fs, info->loc, ent);

    return done > 0 ? (ssize_t)done : -1;
}

/* Drop a file's data. Only length 0 is supported: free the whole cluster chain
 * and clear the size / first-cluster fields in the on-disk directory entry.
 * Called from vfs_open() for O_TRUNC and reachable later via ftruncate(). */
static int fat16_truncate(struct inode *inode, u64 length)
{
    struct fat16 *fs = fs_of(inode);
    struct fat16_inode_info *info = inode->private_data;
    struct fat16_directory *ent = &info->ent;

    if (is_dir(ent) || fs->dev->read_only) return -1;
    if (length != 0) return -1; /* growing / partial truncate not implemented */

    if (inode->size == 0 && ent->first_cluster_low == 0)
        return 0; /* already empty - nothing on disk to touch */

    if (fat16_cluster_valid(ent->first_cluster_low))
        fat16_chain_free(fs, ent->first_cluster_low);

    ent->first_cluster_low = 0;
    ent->file_size_bytes = 0;
    inode->size = 0;
    fat16_entry_touch(ent);
    return fat16_dir_write(fs, info->loc, ent);
}

/* ===== namespace ops ===== */

static int fat16_create(struct inode *dir, struct dentry *dentry, int mode)
{
    (void)mode;
    struct fat16 *fs = fs_of(dir);
    if (fs->dev->read_only) return -1;

    struct fat16_directory ent;
    if (fat16_fill_entry(&ent, dentry->name, FAT16_ATTR_ARCHIVE) != 0) return -1;

    u16 dir_first = ent_of(dir)->first_cluster_low;
    if (fat16_dir_find(fs, dir_first, dentry->name, &(struct fat16_dir_loc){0}, NULL) == 0)
        return -1; /* already exists */

    struct fat16_dir_loc loc;
    if (fat16_dir_alloc_slot(fs, dir_first, &loc) != 0) return -1;
    if (fat16_dir_write(fs, loc, &ent) != 0) return -1;

    struct inode *inode = fat16_make_inode(dir->sb, &ent, loc);
    if (!inode) return -1;

    dentry->inode = inode;
    return 0;
}

static int fat16_mkdir(struct inode *dir, struct dentry *dentry, int mode)
{
    (void)mode;
    struct fat16 *fs = fs_of(dir);
    if (fs->dev->read_only) return -1;

    struct fat16_directory ent;
    if (fat16_fill_entry(&ent, dentry->name, FAT16_ATTR_DIRECTORY) != 0) return -1;

    u16 dir_first = ent_of(dir)->first_cluster_low;
    if (fat16_dir_find(fs, dir_first, dentry->name, &(struct fat16_dir_loc){0}, NULL) == 0)
        return -1;

    u16 cluster;
    if (fat16_cluster_alloc(fs, &cluster) != 0) return -1;
    ent.first_cluster_low = cluster;

    /* Zero the new directory cluster, then lay down "." and "..". */
    char buf[FAT16_SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    u32 base = fat16_cluster_to_lba(fs, cluster);
    for (u8 s = 1; s < fs->header.sectors_per_cluster; s++)
        if (blk_dev_write(fs->dev, base + s, buf, 1) != 0) return -1;

    struct fat16_directory dot;
    memset(&dot, 0, sizeof(dot));
    memset(dot.name, ' ', sizeof(dot.name));
    memset(dot.extension, ' ', sizeof(dot.extension));
    dot.name[0] = '.';
    dot.attributes = FAT16_ATTR_DIRECTORY;
    dot.first_cluster_low = cluster;
    fat16_entry_stamp(&dot);

    struct fat16_directory dotdot = dot;
    dotdot.name[1] = '.';
    dotdot.first_cluster_low = dir_first; /* 0 == root, per the FAT convention */

    memcpy(buf, &dot, sizeof(dot));
    memcpy(buf + FAT16_DIR_ENTRY_SIZE, &dotdot, sizeof(dotdot));
    if (blk_dev_write(fs->dev, base, buf, 1) != 0) return -1;

    struct fat16_dir_loc loc;
    if (fat16_dir_alloc_slot(fs, dir_first, &loc) != 0) goto undo;
    if (fat16_dir_write(fs, loc, &ent) != 0) goto undo;

    struct inode *inode = fat16_make_inode(dir->sb, &ent, loc);
    if (!inode) goto undo;

    dentry->inode = inode;
    return 0;

undo:
    fat16_chain_free(fs, cluster);
    return -1;
}

static int fat16_unlink(struct inode *dir, struct dentry *dentry)
{
    struct fat16 *fs = fs_of(dir);
    if (fs->dev->read_only) return -1;

    struct fat16_dir_loc loc;
    struct fat16_directory ent;
    if (fat16_dir_find(fs, ent_of(dir)->first_cluster_low, dentry->name, &loc, &ent) != 0)
        return -1;

    if (is_dir(&ent)) return -1; /* directories go through rmdir */

    if (fat16_cluster_valid(ent.first_cluster_low))
        fat16_chain_free(fs, ent.first_cluster_low);

    ent.name[0] = FAT16_DIR_ENTRY_DELETED;
    return fat16_dir_write(fs, loc, &ent);
}

static int fat16_rmdir(struct inode *dir, struct dentry *dentry)
{
    struct fat16 *fs = fs_of(dir);
    if (fs->dev->read_only) return -1;

    struct fat16_dir_loc loc;
    struct fat16_directory ent;
    if (fat16_dir_find(fs, ent_of(dir)->first_cluster_low, dentry->name, &loc, &ent) != 0)
        return -1;

    if (!is_dir(&ent)) return -1;
    if (!fat16_dir_empty(fs, ent.first_cluster_low)) return -1; /* not empty */

    if (fat16_cluster_valid(ent.first_cluster_low))
        fat16_chain_free(fs, ent.first_cluster_low);

    ent.name[0] = FAT16_DIR_ENTRY_DELETED;
    return fat16_dir_write(fs, loc, &ent);
}

/* ===== registration ===== */

int fat16_init()
{
    struct fs_type *type = zalloc(sizeof(*type));
    if (!type) return -1;

    strcpy(type->name, name);
    type->mount = fat16_mount_fs;

    return fs_register(type);
}

fs_initcall(fat16_init);
