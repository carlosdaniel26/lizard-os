#include <lizard/blk_dev.h>
#include <lizard/fat16_helpers.h>
#include <lizard/ktime.h>
#include <nolibc/string.h>

/* ===== directory entry / name helpers ===== */

bool fat16_entry_valid(const struct fat16_directory *entry)
{
    if (entry->name[0] == FAT16_DIR_ENTRY_DELETED || entry->name[0] == FAT16_DIR_ENTRY_END)
        return false;

    if (entry->attributes == FAT16_ATTR_LFN)
        return false;

    if (entry->attributes & FAT16_ATTR_VOLUME_ID)
        return false;

    for (int i = 0; i < 8; i++)
        if (entry->name[i] < 0x20 || entry->name[i] > 0x7E)
            return false;

    for (int i = 0; i < 3; i++)
        if (entry->extension[i] < 0x20 || entry->extension[i] > 0x7E)
            return false;

    return true;
}

void fat16_name_to_str(const u8 name[8], const u8 ext[3], char *out)
{
    int pos = 0;

    for (int i = 0; i < 8; i++)
        if (name[i] != ' ') out[pos++] = name[i];

    if (ext[0] != ' ')
    {
        out[pos++] = '.';
        for (int i = 0; i < 3; i++)
            if (ext[i] != ' ') out[pos++] = ext[i];
    }

    out[pos] = '\0';
}

static char to_upper(char c)
{
    return (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
}

bool fat16_str_to_name(const char *in, u8 name[8], u8 ext[3])
{
    memset(name, ' ', 8);
    memset(ext, ' ', 3);

    while (*in == '/') in++;

    int n = 0;
    while (*in && *in != '.')
    {
        if (n >= 8) return false;
        name[n++] = to_upper(*in++);
    }
    if (n == 0) return false;

    if (*in == '.')
    {
        in++;
        int e = 0;
        while (*in)
        {
            if (e >= 3) return false;
            ext[e++] = to_upper(*in++);
        }
    }

    return true;
}

int fat16_name_cmp(const char *path, const struct fat16_directory *entry)
{
    char entry_name[FAT16_NAME_MAX];
    fat16_name_to_str(entry->name, entry->extension, entry_name);

    while (*path == '/') path++;

    return strcasecmp(path, entry_name);
}

bool fat16_is_dot(const struct fat16_directory *entry)
{
    return memcmp(entry->name, ".       ", 8) == 0 ||
           memcmp(entry->name, "..      ", 8) == 0;
}

/* Pack the wall clock into the FAT (date, time) 16-bit fields. */
static void fat16_dos_now(u16 *date, u16 *time)
{
    struct time_spec now = time_now();

    i32 y = timespec_get_year(&now);
    i32 mo = timespec_get_month(&now);
    i32 d = timespec_get_day(&now);
    i32 h = timespec_get_hour(&now);
    i32 mi = timespec_get_min(&now);
    i32 s = timespec_get_sec(&now);

    if (y < 1980) y = 1980;

    *date = (u16)(((y - 1980) << 9) | (mo << 5) | d);
    *time = (u16)((h << 11) | (mi << 5) | (s >> 1));
}

void fat16_entry_stamp(struct fat16_directory *entry)
{
    u16 date, time;
    fat16_dos_now(&date, &time);

    entry->creation_time_tenths = 0;
    entry->creation_time = time;
    entry->creation_date = date;
    entry->last_access_date = date;
    entry->write_time = time;
    entry->write_date = date;
}

void fat16_entry_touch(struct fat16_directory *entry)
{
    u16 date, time;
    fat16_dos_now(&date, &time);

    entry->write_time = time;
    entry->write_date = date;
    entry->last_access_date = date;
}

/* ===== FAT table ===== */

int fat16_fat_get(struct fat16 *fs, u16 cluster, u16 *out)
{
    u32 byte = (u32)cluster * FAT16_FAT_ENTRY_SIZE;
    u32 sector = fs->fat_start_lba + byte / fs->header.bytes_per_sector;
    u32 off = byte % fs->header.bytes_per_sector;

    char buf[FAT16_SECTOR_SIZE];
    if (blk_dev_read(fs->dev, sector, buf, 1) != 0) return -1;

    memcpy(out, buf + off, sizeof(*out));
    return 0;
}

int fat16_fat_set(struct fat16 *fs, u16 cluster, u16 value)
{
    u32 byte = (u32)cluster * FAT16_FAT_ENTRY_SIZE;
    u32 off = byte % fs->header.bytes_per_sector;

    for (u8 f = 0; f < fs->header.num_fats; f++)
    {
        u32 sector = fs->fat_start_lba + (u32)f * fs->header.fat_size_16 +
                     byte / fs->header.bytes_per_sector;

        char buf[FAT16_SECTOR_SIZE];
        if (blk_dev_read(fs->dev, sector, buf, 1) != 0) return -1;
        memcpy(buf + off, &value, sizeof(value));
        if (blk_dev_write(fs->dev, sector, buf, 1) != 0) return -1;
    }

    return 0;
}

u16 fat16_next_cluster(struct fat16 *fs, u16 cluster)
{
    u16 next;
    if (fat16_fat_get(fs, cluster, &next) != 0) return FAT16_EOC;
    return next;
}

u32 fat16_cluster_to_lba(struct fat16 *fs, u16 cluster)
{
    if (cluster < FAT16_CLUSTER_MIN) return 0;
    return ((u32)(cluster - FAT16_CLUSTER_MIN) * fs->header.sectors_per_cluster) + fs->data_region_lba;
}

int fat16_cluster_alloc(struct fat16 *fs, u16 *out)
{
    u32 last = fs->total_clusters + 1; /* data clusters are 2 .. N+1 */
    if (last > FAT16_CLUSTER_MAX) last = FAT16_CLUSTER_MAX;

    for (u16 c = FAT16_CLUSTER_MIN; c <= last; c++)
    {
        u16 v;
        if (fat16_fat_get(fs, c, &v) != 0) return -1;

        if (v == FAT16_FREE_CLUSTER)
        {
            if (fat16_fat_set(fs, c, FAT16_EOC) != 0) return -1;
            *out = c;
            return 0;
        }
    }

    return -1; /* volume full */
}

int fat16_chain_free(struct fat16 *fs, u16 first)
{
    for (u16 c = first; fat16_cluster_valid(c);)
    {
        u16 next;
        if (fat16_fat_get(fs, c, &next) != 0) return -1;
        if (fat16_fat_set(fs, c, FAT16_FREE_CLUSTER) != 0) return -1;
        c = next;
    }

    return 0;
}

/* ===== directory slot access ===== */

int fat16_dir_read(struct fat16 *fs, struct fat16_dir_loc loc, struct fat16_directory *out)
{
    char buf[FAT16_SECTOR_SIZE];
    if (blk_dev_read(fs->dev, loc.lba, buf, 1) != 0) return -1;

    memcpy(out, buf + loc.off, sizeof(*out));
    return 0;
}

int fat16_dir_write(struct fat16 *fs, struct fat16_dir_loc loc, const struct fat16_directory *ent)
{
    char buf[FAT16_SECTOR_SIZE];
    if (blk_dev_read(fs->dev, loc.lba, buf, 1) != 0) return -1;

    memcpy(buf + loc.off, ent, sizeof(*ent));
    return blk_dev_write(fs->dev, loc.lba, buf, 1) == 0 ? 0 : -1;
}

/*
 * Low-level slot scanner: calls slot(first_name_byte, loc, ctx) for every 32-
 * byte directory slot. slot() returns 0 to keep scanning or non-zero to stop
 * (that value is returned). Root directory when dir_first == 0, else the chain.
 */
typedef int (*fat16_slot_cb)(u8 c0, struct fat16_dir_loc loc, void *ctx);

static int fat16_dir_scan(struct fat16 *fs, u16 dir_first, fat16_slot_cb slot, void *ctx)
{
    const u32 per_sector = fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE;
    char buf[FAT16_SECTOR_SIZE];

    if (dir_first == 0)
    {
        for (u32 s = 0; s < fs->root_dir_sectors; s++)
        {
            u32 lba = fs->root_dir_lba + s;
            if (blk_dev_read(fs->dev, lba, buf, 1) != 0) return -1;

            for (u32 i = 0; i < per_sector; i++)
            {
                struct fat16_dir_loc loc = { lba, i * FAT16_DIR_ENTRY_SIZE };
                int r = slot((u8)buf[loc.off], loc, ctx);
                if (r) return r;
            }
        }
        return 0;
    }

    for (u16 cluster = dir_first; fat16_cluster_valid(cluster);
         cluster = fat16_next_cluster(fs, cluster))
    {
        u32 base = fat16_cluster_to_lba(fs, cluster);

        for (u8 s = 0; s < fs->header.sectors_per_cluster; s++)
        {
            u32 lba = base + s;
            if (blk_dev_read(fs->dev, lba, buf, 1) != 0) return -1;

            for (u32 i = 0; i < per_sector; i++)
            {
                struct fat16_dir_loc loc = { lba, i * FAT16_DIR_ENTRY_SIZE };
                int r = slot((u8)buf[loc.off], loc, ctx);
                if (r) return r;
            }
        }
    }

    return 0;
}

struct find_ctx {
    struct fat16 *fs;
    const char *name;
    struct fat16_dir_loc *loc;
    struct fat16_directory *out;
};

static int find_slot(u8 c0, struct fat16_dir_loc loc, void *ctxp)
{
    struct find_ctx *ctx = ctxp;

    if (c0 == FAT16_DIR_ENTRY_END) return -1; /* not found */
    if (c0 == FAT16_DIR_ENTRY_DELETED) return 0;

    struct fat16_directory ent;
    if (fat16_dir_read(ctx->fs, loc, &ent) != 0) return -1;

    if (!fat16_entry_valid(&ent)) return 0;
    if (fat16_name_cmp(ctx->name, &ent) != 0) return 0;

    *ctx->loc = loc;
    if (ctx->out) *ctx->out = ent;
    return 1;
}

int fat16_dir_find(struct fat16 *fs, u16 dir_first, const char *name,
                   struct fat16_dir_loc *loc, struct fat16_directory *out)
{
    struct find_ctx ctx = { fs, name, loc, out };
    return fat16_dir_scan(fs, dir_first, find_slot, &ctx) == 1 ? 0 : -1;
}

static int nonempty_slot(u8 c0, struct fat16_dir_loc loc, void *ctxp)
{
    struct fat16 *fs = ctxp;

    if (c0 == FAT16_DIR_ENTRY_END) return 0;
    if (c0 == FAT16_DIR_ENTRY_DELETED) return 0;

    struct fat16_directory ent;
    if (fat16_dir_read(fs, loc, &ent) != 0) return -1;

    if (fat16_entry_valid(&ent) && !fat16_is_dot(&ent)) return 1; /* found a child */
    return 0;
}

bool fat16_dir_empty(struct fat16 *fs, u16 first_cluster)
{
    /* Exactly 0 == full scan, no child found. 1 == child, -1 == I/O error. */
    return fat16_dir_scan(fs, first_cluster, nonempty_slot, fs) == 0;
}

static int free_slot(u8 c0, struct fat16_dir_loc loc, void *ctxp)
{
    if (c0 != FAT16_DIR_ENTRY_END && c0 != FAT16_DIR_ENTRY_DELETED) return 0;

    *(struct fat16_dir_loc *)ctxp = loc;
    return 1;
}

int fat16_dir_alloc_slot(struct fat16 *fs, u16 dir_first, struct fat16_dir_loc *loc)
{
    if (fat16_dir_scan(fs, dir_first, free_slot, loc) == 1)
        return 0;

    if (dir_first == 0)
        return -1; /* fixed root directory is full */

    /* Extend the directory chain by one zeroed cluster. */
    u16 last = dir_first;
    for (u16 c = fat16_next_cluster(fs, last); fat16_cluster_valid(c);
         c = fat16_next_cluster(fs, c))
        last = c;

    u16 nc;
    if (fat16_cluster_alloc(fs, &nc) != 0) return -1;
    if (fat16_fat_set(fs, last, nc) != 0) return -1;

    char buf[FAT16_SECTOR_SIZE];
    memset(buf, 0, sizeof(buf));
    u32 base = fat16_cluster_to_lba(fs, nc);
    for (u8 s = 0; s < fs->header.sectors_per_cluster; s++)
        if (blk_dev_write(fs->dev, base + s, buf, 1) != 0) return -1;

    loc->lba = base;
    loc->off = 0;
    return 0;
}

/* ===== directory iteration ===== */

static int walk_sector(struct fat16 *fs, u32 lba, u32 per_sector, u32 *offset,
                       fat16_dir_cb cb, void *ctx, bool *ended)
{
    char buf[FAT16_SECTOR_SIZE];
    if (blk_dev_read(fs->dev, lba, buf, 1) != 0) return -1;

    for (u32 i = 0; i < per_sector; i++, *offset += FAT16_DIR_ENTRY_SIZE)
    {
        const struct fat16_directory *e =
            (const struct fat16_directory *)(buf + i * FAT16_DIR_ENTRY_SIZE);

        if (e->name[0] == FAT16_DIR_ENTRY_END)
        {
            *ended = true;
            return 0;
        }

        struct fat16_dir_loc loc = { lba, i * FAT16_DIR_ENTRY_SIZE };
        int r = cb(e, *offset, loc, ctx);
        if (r) return r;
    }

    return 0;
}

int fat16_walk_dir(struct fat16 *fs, u16 first_cluster, fat16_dir_cb cb, void *ctx)
{
    const u32 per_sector = fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE;
    bool ended = false;
    u32 offset = 0;
    int r;

    if (first_cluster == 0)
    {
        for (u32 s = 0; s < fs->root_dir_sectors && !ended; s++)
        {
            r = walk_sector(fs, fs->root_dir_lba + s, per_sector, &offset, cb, ctx, &ended);
            if (r) return r;
        }
        return 0;
    }

    for (u16 cluster = first_cluster; fat16_cluster_valid(cluster) && !ended;
         cluster = fat16_next_cluster(fs, cluster))
    {
        u32 base = fat16_cluster_to_lba(fs, cluster);

        for (u8 s = 0; s < fs->header.sectors_per_cluster && !ended; s++)
        {
            r = walk_sector(fs, base + s, per_sector, &offset, cb, ctx, &ended);
            if (r) return r;
        }
    }

    return 0;
}
