#include <lizard/elf.h>
#include <lizard/file.h>
#include <lizard/gdt.h>
#include <lizard/kmalloc.h>
#include <lizard/loader.h>
#include <lizard/pgtable.h>
#include <nolibc/stdio.h>
#include <nolibc/string.h>
#include <lizard/task.h>
#include <lizard/vfs.h>
#include <lizard/vmm.h>

extern u64 hhdm_offset;

#define MAX_PHDRS 16

/* Read exactly `len` bytes from `file` at absolute `off` into `dst`.
 * Returns 0 on success, -1 on short read / error. */
static int read_at(struct file *file, u64 off, void *dst, u64 len)
{
    u8 *p = dst;
    file->offset = off;
    while (len)
    {
        ssize_t n = vfs_read(file, p, len);
        if (n <= 0)
            return -1;
        p += n;
        len -= (u64)n;
    }
    return 0;
}

/* Load an ET_EXEC ELF straight from the VFS into `task`'s address space. The
 * file is streamed segment by segment - no whole-image buffer - so multi-MB
 * binaries (doom) don't need a giant kmalloc. */
int load_elf(struct file *file, struct task *task)
{
    elf64_ehdr_t ehdr;

    if (read_at(file, 0, &ehdr, sizeof(ehdr)) != 0)
    {
        kprintf("load_elf: short read on ELF header\n");
        return -1;
    }

    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 || ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 || ehdr.e_ident[EI_MAG3] != ELFMAG3)
    {
        kprintf("Not a valid ELF file\n");
        return -1;
    }

    if (ehdr.e_type != ET_EXEC)
    {
        kprintf("Not an executable ELF file\n");
        return -1;
    }

    if (ehdr.e_phnum > MAX_PHDRS)
    {
        kprintf("load_elf: too many program headers (%u)\n", ehdr.e_phnum);
        return -1;
    }

    elf64_phdr_t phdr[MAX_PHDRS];
    if (read_at(file, ehdr.e_phoff, phdr, (u64)ehdr.e_phnum * sizeof(elf64_phdr_t)) != 0)
    {
        kprintf("load_elf: short read on program headers\n");
        return -1;
    }

    vaddr_t old_pml4 = (vaddr_t)current_pml4;
    vmm_switch_pml4(task->pml4);

    int rc = 0;
    for (int i = 0; i < ehdr.e_phnum; i++)
    {
        if (phdr[i].p_type != PT_LOAD || phdr[i].p_memsz == 0)
            continue;

        for (u64 j = 0; j < phdr[i].p_memsz; j += PAGE_SIZE)
            vmm_alloc(task->pml4, phdr[i].p_vaddr + j,
                      PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

        /* buddy_alloc hands back dirty pages: zero the whole segment so .bss
         * (the [filesz, memsz) tail) starts clear, then stream the file bytes
         * over it. */
        memset((void *)phdr[i].p_vaddr, 0, phdr[i].p_memsz);

        if (phdr[i].p_filesz &&
            read_at(file, phdr[i].p_offset, (void *)phdr[i].p_vaddr, phdr[i].p_filesz) != 0)
        {
            kprintf("load_elf: short read on segment %d\n", i);
            rc = -1;
            break;
        }
    }

    task->regs.rip = ehdr.e_entry;

    vmm_switch_pml4(old_pml4);
    return rc;
}

int spawn(const char *path)
{
    struct file *file = vfs_open(path, O_RDONLY);
    if (!file)
        return -1;

    struct task *t = zalloc(sizeof(struct task));
    if (!t)
    {
        vfs_close(file);
        return -1;
    }

    const char *name = path;
    for (const char *p = path; *p; p++)
        if (*p == '/') name = p + 1;

    task_create(t, (void (*)(void))0, name, 1, TASK_USER);
    t->on_heap = true;

    if (load_elf(file, t) != 0)
    {
        vfs_close(file);
        t->reaped = true;                 /* unparented - reaper frees it */
        t->state = TASK_STATE_TERMINATED; /* reaper reclaims pml4 / kstack / t */
        return -1;
    }

    vfs_close(file);

    /* Join the process tree so the caller can waitpid() on us. */
    t->parent = current_task;
    list_add_tail(&t->sibling, &current_task->children);

    return (int)t->pid;
}
