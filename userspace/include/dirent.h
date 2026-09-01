#pragma once

/* Layout mirrors the kernel's struct dirent (nolibc/dirent.h) exactly -
 * SYS_readdir memcpy's whole records into the caller's array. */

#define DT_UNKNOWN 0
#define DT_DIR     4
#define DT_REG     8

struct dirent {
    unsigned long long d_ino;
    long long          d_off;    /* offset of the next entry */
    unsigned short     d_reclen;
    unsigned char      d_type;   /* DT_* */
    char               d_name[256];
};

/* Fill up to `max` entries from an open directory fd; returns the count
 * written (0 at end of directory) or -errno. */
int sys_readdir(int fd, struct dirent *buf, int max);
