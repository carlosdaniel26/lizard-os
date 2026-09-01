#pragma once
#include <abi/syscall.h> /* not strictly needed, keeps flag values in one family */

#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0040
#define O_TRUNC  0x0200
#define O_APPEND 0x0400
#define O_BINARY 0x0000 /* no text mode here */

int open(const char *path, int flags, ...);
