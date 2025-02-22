#include <multiboot2.h>
#include <kernel/terminal/terminal.h>
#include <stdio.h>

extern uint64_t mem_ammount_kb;

void process_multiboot2_tags(unsigned long magic_number, unsigned long addr)
{
	kprintf("Processing multiboot...\n");

	if (magic_number != MULTIBOOT2_MAGIC)
	{
		kprintf("Invalid magic number: 0x%x\n", (unsigned) magic_number);
		asm("hlt");
		return;
	}

	if (addr & 7)
	{
		kprintf("Unaligned mbi: 0x%x\n", addr);
		return;
	}

	unsigned size = *(unsigned *) addr;
	kprintf("MBI size: %u\n", size);

	terminal_clean();

	struct multiboot_tag *tag;
	for (tag = (struct multiboot_tag *) (addr + 8);
		 tag->type != MULTIBOOT_TAG_TYPE_END;
		 tag = (struct multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7)))
	{
		kprintf("Tag 0x%x, Size 0x%x\n", tag->type, tag->size);

		switch (tag->type)
		{
			case MULTIBOOT_TAG_TYPE_CMDLINE:
				kprintf("Command line = %s\n", ((struct multiboot_tag_string *) tag)->string);
				break;
			case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
				kprintf("Boot loader name = %s\n", ((struct multiboot_tag_string *) tag)->string);
				break;
			case MULTIBOOT_TAG_TYPE_MODULE:
				kprintf("Module at 0x%x-0x%x. Command line %s\n",
					   ((struct multiboot_tag_module *) tag)->mod_start,
					   ((struct multiboot_tag_module *) tag)->mod_end,
					   ((struct multiboot_tag_module *) tag)->cmdline);
				break;
			case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
				kprintf("mem_lower = %uKB, mem_upper = %uKB\n",
					   ((struct multiboot_tag_basic_meminfo *) tag)->mem_lower,
					   ((struct multiboot_tag_basic_meminfo *) tag)->mem_upper);

					mem_ammount_kb =
						((struct multiboot_tag_basic_meminfo *) tag)->mem_lower
						+
						((struct multiboot_tag_basic_meminfo *) tag)->mem_upper;


				break;
			case MULTIBOOT_TAG_TYPE_BOOTDEV:
				kprintf("Boot device 0x%x,%u,%u\n",
					   ((struct multiboot_tag_bootdev *) tag)->biosdev,
					   ((struct multiboot_tag_bootdev *) tag)->slice,
					   ((struct multiboot_tag_bootdev *) tag)->part);
				break;
			case MULTIBOOT_TAG_TYPE_MMAP:
			{
				struct multiboot_tag_mmap *mmap_tag = (struct multiboot_tag_mmap *) tag;
				struct multiboot_mmap_entry *mmap;
				kprintf("mmap\n");
				for (mmap = mmap_tag->entries;
					 (unsigned char *) mmap < (unsigned char *) tag + tag->size;
					 mmap = (struct multiboot_mmap_entry *) ((unsigned long) mmap + mmap_tag->entry_size))
				{
					kprintf(" addr = 0x%x%x, len = 0x%x%x, t = 0x%x\n",
						   (unsigned) (mmap->addr >> 32),
						   (unsigned) (mmap->addr & 0xffffffff),
						   (unsigned) (mmap->len >> 32),
						   (unsigned) (mmap->len & 0xffffffff),
						   (unsigned) mmap->type);

				}
			}
			break;


			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
			{
				struct multiboot_tag_framebuffer *fb_tag = (struct multiboot_tag_framebuffer *) tag;
				struct multiboot_tag_framebuffer_common *cfb_tag = (struct multiboot_tag_framebuffer_common *) tag;

				uint32_t width = cfb_tag->width;
				uint32_t height = cfb_tag->height;

				uint32_t *fb = (uint32_t*) cfb_tag->framebuffer_addr;
				for (uint32_t y = 0; y < height; y++) {
					for (uint32_t x = 0; x < width; x++) {
						fb[y * width + x] = 0xFF0000FF; // BGRA
					}
				}
				while(1){}
			}
			break;

			default:
				/*kprintf("Unknown tag type: 0x%x\n", tag->type);*/
				break;
		}
	}
}