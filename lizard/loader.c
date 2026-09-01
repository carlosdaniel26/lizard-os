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
#define MAX_ARGS  32 /* argv entries handed to a new task, argv[0] included */

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

/* Copy into the new task's address space through its HHDM alias - the task's
 * pml4 is not the live CR3 during load, so its user VAs cannot be touched
 * directly. Splits at page boundaries. */
static void ucopy(struct task *task, u64 uva, const void *src, size_t n)
{
    const u8 *s = src;
    while (n)
    {
        u8 *k = pgtable_kva(task->pml4, uva);
        size_t chunk = PAGE_SIZE - (uva & 0xFFF);
        if (chunk > n)
            chunk = n;
        memcpy(k, s, chunk);
        s += chunk;
        uva += chunk;
        n -= chunk;
    }
}

static void uput64(struct task *task, u64 uva, u64 val)
{
    ucopy(task, uva, &val, sizeof(val));
}

/* Build the initial user stack: the arg strings, then a System V x86-64
 * startup block [ argc | argv[0..argc-1] | NULL | envp NULL | auxv AT_NULL ],
 * with the returned rsp 16-byte aligned as _start expects. */
static u64 setup_user_stack(struct task *task, char *const argv[])
{
    int argc = 0;
    if (argv)
        while (argv[argc] && argc < MAX_ARGS)
            argc++;

    u64 sp = task->regs.rsp;
    u64 argp[MAX_ARGS];

    for (int i = argc - 1; i >= 0; i--)
    {
        size_t len = strlen(argv[i]) + 1;
        sp -= len;
        ucopy(task, sp, argv[i], len);
        argp[i] = sp;
    }

    sp &= ~0xFUL; /* realign after the byte-granular string copies */

    /* slots below: argc, argv[], NULL, envp NULL, auxv AT_NULL. Pad so the
     * final rsp (pointing at argc) stays 16-aligned. */
    if ((argc & 1) == 1)
        sp -= 8;

    sp -= 8; uput64(task, sp, 0); /* auxv: AT_NULL     */
    sp -= 8; uput64(task, sp, 0); /* envp[0] = NULL    */
    sp -= 8; uput64(task, sp, 0); /* argv[argc] = NULL */
    for (int i = argc - 1; i >= 0; i--)
    {
        sp -= 8;
        uput64(task, sp, argp[i]);
    }
    sp -= 8; uput64(task, sp, (u64)argc);

    return sp;
}

/* Load an ET_EXEC ELF straight from the VFS into `task`'s address space. The
 * file is streamed segment by segment - no whole-image buffer - so multi-MB
 * binaries (doom) don't need a giant kmalloc. argv (NULL-terminated, may be
 * NULL) is copied onto the new task's stack for _start. */
int load_elf(struct file *file, struct task *task, char *const argv[])
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

    /* The task's pml4 is not the live CR3, so map each PT_LOAD page into it and
     * populate the page through its freshly allocated frame's HHDM alias -
     * never through the target VA. This keeps load preemption-safe: no CR3
     * borrow that a context switch could quietly undo mid-load. */
    int rc = 0;
    for (int i = 0; i < ehdr.e_phnum; i++)
    {
        elf64_phdr_t *ph = &phdr[i];
        if (ph->p_type != PT_LOAD || ph->p_memsz == 0)
            continue;

        if (ph->p_vaddr & 0xFFF)
        {
            kprintf("load_elf: segment %d vaddr not page aligned\n", i);
            rc = -1;
            break;
        }

        for (u64 j = 0; j < ph->p_memsz; j += PAGE_SIZE)
        {
            void *frame = vmm_alloc(task->pml4, ph->p_vaddr + j,
                                    PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);

            /* buddy_alloc hands back dirty pages: zero every page so the .bss
             * tail [filesz, memsz) starts clear. */
            memset(frame, 0, PAGE_SIZE);

            u64 fill = (ph->p_filesz > j) ? ph->p_filesz - j : 0;
            if (fill > PAGE_SIZE)
                fill = PAGE_SIZE;
            if (fill && read_at(file, ph->p_offset + j, frame, fill) != 0)
            {
                kprintf("load_elf: short read on segment %d\n", i);
                rc = -1;
                break;
            }
        }
        if (rc)
            break;
    }

    task->regs.rip = ehdr.e_entry;
    if (rc == 0)
        task->regs.rsp = setup_user_stack(task, argv);

    return rc;
}

int spawn(const char *path, char *const argv[])
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

    /* Default argv is just the program name so argv[0] is always valid. */
    char *defargv[2] = {(char *)name, NULL};
    if (!argv || !argv[0])
        argv = defargv;

    if (load_elf(file, t, argv) != 0)
    {
        vfs_close(file);
        t->reaped = true;                 /* unparented - reaper frees it */
        t->state = TASK_STATE_TERMINATED; /* reaper reclaims pml4 / kstack / t */
        return -1;
    }

    vfs_close(file);

    /* Join the process tree so the caller can waitpid() on us, then - with the
     * image and stack fully in place - make the task dispatchable. */
    t->parent = current_task;
    list_add_tail(&t->sibling, &current_task->children);
    task_set_ready(t);

    return (int)t->pid;
}
