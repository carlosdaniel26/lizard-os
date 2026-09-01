#pragma once

struct task;

int load_elf(void *buffer, struct task *task);

/* Read an ET_EXEC ELF from `path` (VFS), create a user task for it and make it
 * runnable. Returns the new pid, or -1 on any failure. Does not wait. */
int spawn(const char *path);
