#include <elf.h>
#include <kmalloc.h>
#include <pgtable.h>
#include <stdio.h>
#include <string.h>
#include <task.h>
#include <vmm.h>

extern u64 hhdm_offset;

int load_elf(void *buffer, struct task *task)
{
    elf64_ehdr_t *ehdr = (elf64_ehdr_t *)buffer;

    if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr->e_ident[EI_MAG3] != ELFMAG3)
    {
        kprintf("Not a valid ELF file\n");
        return -1;
    }

    if (ehdr->e_type != ET_EXEC)
    {
        kprintf("Not an executable ELF file\n");
        return -1;
    }

    task->pml4 = pgtable_create();
    
    // Switch to new pml4 to map segments
    u64 *old_pml4 = current_pml4;
    vmm_switch_pml4(task->pml4);

    elf64_phdr_t *phdr = (elf64_phdr_t *)(buffer + ehdr->e_phoff);
    for (int i = 0; i < ehdr->e_phnum; i++)
    {
        if (phdr[i].p_type == PT_LOAD)
        {
            for (u64 j = 0; j < phdr[i].p_memsz; j += PAGE_SIZE)
            {
                vmm_alloc(task->pml4, phdr[i].p_vaddr + j, PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER);
            }
            memcpy((void *)phdr[i].p_vaddr, buffer + phdr[i].p_offset, phdr[i].p_filesz);
        }
    }

    task->regs.rip = ehdr->e_entry;
    vmm_switch_pml4(old_pml4);

    return 0;
}
