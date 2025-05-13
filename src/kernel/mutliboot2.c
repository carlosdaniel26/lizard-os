#include <multiboot2.h>
#include <kernel/terminal/tty.h>
#include <kernel/drivers/framebuffer.h>
#include <stdio.h>

extern uint32_t mem_ammount_kb;

struct multiboot_tag *tag;
struct multiboot_tag_mmap *mmap_tag;

void process_multiboot2_tags(unsigned long magic_number, unsigned long addr)
{

	if (magic_number != MULTIBOOT2_MAGIC)
	{
		asm("hlt");
		return;
	}

	if (addr & 7)
	{
		return;
	}

	tty_clean();
	
	for (tag = (struct multiboot_tag *) (addr + 8);
		 tag->type != MULTIBOOT_TAG_TYPE_END;
		 tag = (struct multiboot_tag *) ((uint8_t *) tag + ((tag->size + 7) & ~7)))
	{

		switch (tag->type)
		{
			case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
					mem_ammount_kb =
						((struct multiboot_tag_basic_meminfo *) tag)->mem_lower
						+
						((struct multiboot_tag_basic_meminfo *) tag)->mem_upper;
				break;

			case MULTIBOOT_TAG_TYPE_MMAP:
			{
				mmap_tag = (struct multiboot_tag_mmap *) tag;
			}
			break;


			case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
			{
				struct multiboot_tag_framebuffer_common *cfb_tag = (struct multiboot_tag_framebuffer_common *) tag;

				setup_framebuffer(cfb_tag);
			}
			break;

			default:
				/*kprintf("Unknown tag type: 0x%x\n", tag->type);*/
				break;
		}
	}
}