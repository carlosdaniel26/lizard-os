#pragma once
/*
 * lizard syscall ABI - shared verbatim by the kernel (lizard/) and userspace
 * (userspace/). Numbers only; no types, no kernel or libc headers.
 *
 * Calling convention (int 0x80):
 *   RAX = syscall number
 *   RDI, RSI, RDX, R10, R8, R9 = args 1..6
 *   RAX = return value on the way out ( >= 0 ok, -errno on failure )
 */

enum {
    SYS_exit   = 0,
    SYS_write  = 1,
    SYS_read   = 2,
    SYS_sleep  = 3,
    SYS_getpid = 4,

    /* --- file I/O (fd >= 3; 0/1/2 are the tty) --------------------------- */
    SYS_open   = 5,  /* (const char *path, int flags)          -> fd | -errno   */
    SYS_close  = 6,  /* (int fd)                               -> 0 | -errno    */
    SYS_lseek  = 7,  /* (int fd, long off, int whence)         -> pos | -errno  */

    /* --- framebuffer / input / time (for doomgeneric & friends) --------- */
    SYS_fb_info   = 8,  /* (struct fb_info *out)              -> 0 | -errno     */
    SYS_fb_blit   = 9,  /* (const void *xrgb, u32 w, u32 h)   -> 0 | -errno     */
    SYS_key_get   = 10, /* ()  -> set-1 scancode (bit7 = release), 0 if none    */
    SYS_uptime_ms = 11, /* ()  -> milliseconds since boot                      */

    /* --- process control ---------------------------------------------- */
    SYS_spawn   = 12, /* (const char *path, char *const argv[]) -> pid | -errno */
    SYS_waitpid = 13, /* (int pid, int *status, int options)    -> pid | -errno */
    SYS_yield   = 14, /* ()                                     -> 0            */

    /* --- directories ------------------------------------------------- */
    SYS_readdir = 15, /* (int fd, struct dirent *buf, int max)  -> count | -errno */

    SYS_NR_MAX
};

/* whence for SYS_lseek */
#define LZ_SEEK_SET 0
#define LZ_SEEK_CUR 1
#define LZ_SEEK_END 2

/* SYS_fb_info output: plain 32-bit fields, shared by kernel and userspace. */
struct fb_info {
    unsigned int width;
    unsigned int height;
    unsigned int pitch; /* bytes per scanline */
    unsigned int bpp;   /* bits per pixel (always 32 for now) */
};
