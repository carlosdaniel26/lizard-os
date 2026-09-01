#pragma once
#include <stddef.h>

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

int   usleep(unsigned usec);
int   unlink(const char *path);
int   isatty(int fd);
int   access(const char *path, int mode);
int   getpid(void);

ssize_t read(int fd, void *buf, size_t n);
ssize_t write(int fd, const void *buf, size_t n);
int     close(int fd);
long    lseek(int fd, long off, int whence);
