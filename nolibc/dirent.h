#pragma once

#include <types.h>

enum dirent_type {
    DT_UNKNOWN = 0,
    DT_DIR     = 4,
    DT_REG     = 8,
};

struct dirent {
    u64 inode;
    off_t offset;				/* next */
    u16 reclen;					/* record lenght */
    u8 type;
    char name[NAME_MAX + 1];
};
