#include <multiboot2.h>
#include <stdio.h>

void process_multiboot_tags(struct multiboot_info_t *multiboot2_info)
{
    struct multiboot2_tag *tag;

    for (tag = (struct multiboot2_tag *)(multiboot2_info + 4);
    	tag->type != MULTIBOOT_TAG_TYPE_END;
		tag = (struct multiboot2_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7))) // magic code to align 8 bytes
	{
		switch(tag->type)
		{
			case MULTIBOOT_TAG_TYPE_END:
			{
				struct multiboot2_tag_mmap *mmap_tag = (struct multiboot2_tag_mmap *)tag;
				struct mmap_entry *entry;

				for (entry = mmap_tag->entries;
					(uint8_t *)entry < (uint8_t *)mmap_tag + mmap_tag->size;
					entry = (struct mmap_entry *)((uint8_t *)entry + mmap_tag->size))
				{
					printf("memory region addr = %llu, len = %llu, type = %u\n", entry->addr, entry->len, entry->type);
				}
				break;
			}
		}
	}
    
}