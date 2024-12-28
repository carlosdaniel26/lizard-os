#include <multiboot2.h>
#include <kernel/terminal/terminal.h>
#include <stdio.h>

extern uint64_t mem_ammount_kb;

void process_multiboot2_tags(unsigned long magic_number, unsigned long addr)
{
    printf("Processing multiboot...\n");

    if (magic_number != MULTIBOOT2_MAGIC)
    {
        printf("Invalid magic number: %u\n", (unsigned) magic_number);
        return;
    }

    if (addr & 7)
    {
        printf("Unaligned mbi: 0x%x\n", addr);
        return;
    }

    unsigned size = *(unsigned *) addr;
    printf("MBI size: %u\n", size);

    terminal_initialize();

    struct multiboot_tag *tag;
    for (tag = (struct multiboot_tag *) (addr + 8);
         tag->type != MULTIBOOT_TAG_TYPE_END;
         tag = (struct multiboot_tag *) ((multiboot_uint8_t *) tag + ((tag->size + 7) & ~7)))
    {
        printf("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);

        switch (tag->type)
        {
            case MULTIBOOT_TAG_TYPE_CMDLINE:
                printf("Command line = %s\n", ((struct multiboot_tag_string *) tag)->string);
                break;
            case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                printf("Boot loader name = %s\n", ((struct multiboot_tag_string *) tag)->string);
                break;
            case MULTIBOOT_TAG_TYPE_MODULE:
                printf("Module at 0x%x-0x%x. Command line %s\n",
                       ((struct multiboot_tag_module *) tag)->mod_start,
                       ((struct multiboot_tag_module *) tag)->mod_end,
                       ((struct multiboot_tag_module *) tag)->cmdline);
                break;
            case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
                printf("mem_lower = %uKB, mem_upper = %uKB\n",
                       ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
                       ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);

                    mem_ammount_kb = 
                        ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower
                        +
                        ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper;

                
                break;
            case MULTIBOOT_TAG_TYPE_BOOTDEV:
                printf("Boot device 0x%x,%u,%u\n",
                       ((struct multiboot_tag_bootdev *) tag)->biosdev,
                       ((struct multiboot_tag_bootdev *) tag)->slice,
                       ((struct multiboot_tag_bootdev *) tag)->part);
                break;
            case MULTIBOOT_TAG_TYPE_MMAP:
            {
                struct multiboot_tag_mmap *mmap_tag = (struct multiboot_tag_mmap *) tag;
                struct multiboot_mmap_entry *mmap;
                printf("mmap\n");
                for (mmap = mmap_tag->entries;
                     (unsigned char *) mmap < (unsigned char *) tag + tag->size;
                     mmap = (struct multiboot_mmap_entry *) ((unsigned long) mmap + mmap_tag->entry_size))
                {
                    printf(" addr = 0x%x%x, len = 0x%x%x, t = 0x%x\n",
                           (unsigned) (mmap->addr >> 32),
                           (unsigned) (mmap->addr & 0xffffffff),
                           (unsigned) (mmap->len >> 32),
                           (unsigned) (mmap->len & 0xffffffff),
                           (unsigned) mmap->type);  

                }
            }
            break;
            default:
                //printf("Unknown tag type: 0x%x\n", tag->type);
                break;
        }
    }
}