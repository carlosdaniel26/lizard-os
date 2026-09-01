#pragma once

struct task;
struct file;

/* Stream an ET_EXEC ELF from `file` into `task`'s address space. */
int load_elf(struct file *file, struct task *task);

/* Open `path` (VFS), create a user task for it and make it runnable.
 * Returns the new pid, or -1 on any failure. Does not wait. */
int spawn(const char *path);
