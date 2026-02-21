#pragma once

#include <atomic.h>
#include <list.h>
#include <spinlock.h>
#include <types.h>

typedef struct FsInstance FsInstance;
typedef struct Dentry Dentry;

typedef struct {
    int (*mount)(const char *source, const char *target, const char *fs_name, u64 mountflags,
                 const void *data);
    int (*umount)(const char *taget, int flags);

    /* file */
    int (*open)(const char *path, int flags, int mode);
    int (*close)(int fd);
    size_t (*read)(int fd, void *buf, size_t count);
    size_t (*write)(int fd, const void *buf, size_t count);
    off_t (*lseek)(int fd, off_t offset, int whence);

    /* directory */
    int (*mkdir)(const char *path, int mode);
    int (*rmdir)(const char *path);
    int (*readdir)(const char *path, void *dir_entry, int *count);

    /* file management */
    int (*rename)(const char *oldpath, const char *newpath);
    int (*unlink)(const char *pathname);
    int (*truncate)(const char *path, off_t length);

    /* permissions */
    int (*symlink)(const char *target, const char *linkpath);
    int (*readlink)(const char *pathname, char *buf, size_t bufsiz);
    int (*link)(const char *oldpath, const char *newpath);

    /* sync */
    int (*sync)(void);
    int (*syncfs)(int fd);

    /* fs specific ops */
    int (*ioctl)(int fd, unsigned long request, void *arg);

    /* instance management */
    int (*get_stats)(FsInstance *instance, void *out_stats);
    int (*check)(FsInstance *instance);
    int (*defrag)(FsInstance);
} FsOps;

typedef struct {
    ListHead list;
    char name[32];
    FsOps ops;
    u32 flags;

    u32 capabilities;
    size_t max_filename_len;
    size_t max_file_size;

    void *private_data;
} FsType;

typedef struct {
    ListHead list;
    const FsType *type;
    spinlock_t lock;

    void *private_data;
    char mount_point[256];
    u64 mount_flags;
    atomic_t ref_count;

    Dentry *root_dentry;
    size_t block_size; /* (NOT BLOCK DEVICE BLOCK SIZE) */
    u64 total_blocks;
    u64 free_blocks;

} FsInstance;

int fstype_register(FsType *type);
int fstype_unregister(FsType *type);
FsType *fstype_find(const char *name);
FsType *fstype_find_locked(const char *name);
int fs_type_count();
