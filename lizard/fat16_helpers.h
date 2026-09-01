#pragma once

#include <nolibc/stdbool.h>
#include <nolibc/types.h>

#include <lizard/fat16_types.h>

/* fat16_walk_dir() callback return codes; negative values propagate as errors. */
#define FAT16_WALK_CONTINUE 0
#define FAT16_WALK_STOP     1

/* On-disk location of a single 32-byte directory slot. */
struct fat16_dir_loc {
    u32 lba; /* sector holding the slot */
    u32 off; /* byte offset of the slot within that sector */
};

static inline bool fat16_cluster_valid(u16 cluster)
{
    return cluster >= FAT16_CLUSTER_MIN && cluster <= FAT16_CLUSTER_MAX;
}

/* ---- directory entry / name helpers ------------------------------------- */

/* True if the slot holds an in-use 8.3 entry (not free/deleted/LFN/volume-
 * label, and the name is printable ASCII). */
bool fat16_entry_valid(const struct fat16_directory *entry);

/* Render an 8.3 name/ext pair as "NAME.EXT" (uppercase, unpadded). `out` must
 * hold at least FAT16_NAME_MAX bytes. */
void fat16_name_to_str(const u8 name[8], const u8 ext[3], char *out);

/* Parse "name.ext" into space-padded, upper-cased 8.3 fields. Leading '/'s are
 * skipped. Returns false if it does not fit 8.3. */
bool fat16_str_to_name(const char *in, u8 name[8], u8 ext[3]);

/* strcasecmp of `path` (leading '/'s skipped) against an entry's 8.3 name. */
int fat16_name_cmp(const char *path, const struct fat16_directory *entry);

/* True for the "." and ".." entries. */
bool fat16_is_dot(const struct fat16_directory *entry);

/* Stamp creation/write/access time on a directory entry from the wall clock. */
void fat16_entry_stamp(struct fat16_directory *entry);

/* Update only the write (modification) timestamp. */
void fat16_entry_touch(struct fat16_directory *entry);

/* ---- FAT table -------------------------------------------------------------- */

/* Read / write one FAT slot. fat16_fat_set() mirrors the value into every FAT
 * copy. Return 0 on success, -1 on I/O error. */
int fat16_fat_get(struct fat16 *fs, u16 cluster, u16 *out);
int fat16_fat_set(struct fat16 *fs, u16 cluster, u16 value);

/* Follow a chain: next cluster, or FAT16_EOC on end-of-chain / I/O error. */
u16 fat16_next_cluster(struct fat16 *fs, u16 cluster);

/* First LBA of a data cluster, or 0 for a cluster below FAT16_CLUSTER_MIN. */
u32 fat16_cluster_to_lba(struct fat16 *fs, u16 cluster);

/* Allocate a free cluster, mark it end-of-chain, return it in *out.
 * Returns 0, -1 on I/O error, or -1 when the volume is full. */
int fat16_cluster_alloc(struct fat16 *fs, u16 *out);

/* Free every cluster in the chain starting at `first`. */
int fat16_chain_free(struct fat16 *fs, u16 first);

/* ---- directory slot access ---------------------------------------------- */

/* Find an existing entry by name. On success fills *loc and *out (may be NULL)
 * and returns 0; returns -1 if not found or on I/O error. */
int fat16_dir_find(struct fat16 *fs, u16 dir_first, const char *name,
                   struct fat16_dir_loc *loc, struct fat16_directory *out);

/* True if the directory at first_cluster holds nothing but "." / "..". */
bool fat16_dir_empty(struct fat16 *fs, u16 first_cluster);

/* Find a free slot (deleted or end-of-directory), extending a subdirectory by a
 * zeroed cluster if it is full. The fixed root directory cannot grow. */
int fat16_dir_alloc_slot(struct fat16 *fs, u16 dir_first, struct fat16_dir_loc *loc);

int fat16_dir_read(struct fat16 *fs, struct fat16_dir_loc loc, struct fat16_directory *out);
int fat16_dir_write(struct fat16 *fs, struct fat16_dir_loc loc, const struct fat16_directory *ent);

/* ---- directory iteration ---------------------------------------------------- */

typedef int (*fat16_dir_cb)(const struct fat16_directory *entry, u32 dir_offset,
                            struct fat16_dir_loc loc, void *ctx);

/*
 * Iterate directory entries, invoking cb(entry, running_byte_offset, loc, ctx)
 * for each slot up to the end-of-directory marker. first_cluster == 0 walks the
 * fixed-size root directory; otherwise the cluster chain starting there.
 *
 * Returns cb's non-zero stop code, -1 on I/O error, or 0 if the directory ended
 * before cb asked to stop.
 */
int fat16_walk_dir(struct fat16 *fs, u16 first_cluster, fat16_dir_cb cb, void *ctx);
