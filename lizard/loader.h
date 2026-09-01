#pragma once

struct task;
struct file;

/* Stream an ET_EXEC ELF from `file` into `task`'s address space and lay out
 * its startup stack from argv (NULL-terminated; may be NULL). */
int load_elf(struct file *file, struct task *task, char *const argv[]);

/* Open `path` (VFS), create a user task for it and make it runnable. argv is
 * NULL-terminated with argv[0] the program name; NULL means "just the name".
 * Returns the new pid, or -1 on any failure. Does not wait. */
int spawn(const char *path, char *const argv[]);
