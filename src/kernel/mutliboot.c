#include <multiboot2.h>
#include <stdio.h>

void process_multiboot2_tags(unsigned long magic_number,unsigned long addr)
{
	printf("processing multiboot...\n");
    if (magic_number != MULTIBOOT2_MAGIC)
    {
		printf ("Invalid magic number: %u\n", (unsigned) magic_number);
    }

	if (addr & 7)
    {
		printf ("Unaligned mbi: 0x%x\n", addr);
    }

	unsigned size = *(unsigned *) addr;
	printf("mbi size: %u\n", size);

	struct multiboot2_tag *tag;

	for (tag = (struct multiboot2_tag *) (addr + 8);
		 tag->type != MULTIBOOT_TAG_TYPE_END;
		 tag = (struct multiboot2_tag *) ((uint8_t *)tag
		 								+ ((tag->size + 7) & ~7))
		)
	{
		printf("Tag %u, Size %u\n", tag->type, tag->size);

		switch (tag->type)
        {
          case MULTIBOOT_TAG_TYPE_MMAP:
            {
              struct multiboot2_tag_mmap *mmap_tag = (struct multiboot2_tag_mmap*) tag;
              struct multiboot_mmap_entry *mmap;
              printf("sizeof(mmap_entry): %u\n", sizeof(struct multiboot_mmap_entry));
              printf ("mmap:\n");
        
              for (mmap = mmap_tag->entries;
                    (uint8_t *)mmap < (uint8_t *)tag + tag->size;
                    mmap = (struct multiboot_mmap_entry *)((uint8_t *)mmap + mmap_tag->size))
                printf ("%u base_addr = 0x%x%x"
                        " length = 0x%x%x, type = 0x%x\n",
                        mmap,
                        (mmap->addr >> 32) |
                        (mmap->addr & 0xffffffff),
                        (mmap->len >> 32)|
                        (mmap->len & 0xffffffff),
                        (unsigned) mmap->type);

              printf("\n");
            }
            break;
        }

	}
    
}