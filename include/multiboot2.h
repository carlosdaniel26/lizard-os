#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

#define MULTIBOOT2_MAGIC 0x36d76289
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE			   7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER	   8


struct multiboot_tag {
	uint32_t type;
	uint32_t size;
};

struct multiboot_tag_string {
	uint32_t type;
	uint32_t size;
	char string[0];
};

struct multiboot_tag_module {
	uint32_t type;
	uint32_t size;
	uint32_t mod_start;
	uint32_t mod_end;
	char cmdline[0];
};

struct multiboot_tag_basic_meminfo {
	uint32_t type;
	uint32_t size;
	uint32_t mem_lower;
	uint32_t mem_upper;
};

struct multiboot_tag_bootdev {
	uint32_t type;
	uint32_t size;
	uint32_t biosdev;
	uint32_t slice;
	uint32_t part;
};

struct multiboot_mmap_entry {
	uint64_t addr;
	uint64_t len;
	uint32_t type;
	uint32_t zero;
};

struct multiboot_tag_mmap {
	#define MULTIBOOT_MEMORY_AVAILABLE              1
	#define MULTIBOOT_MEMORY_RESERVED               2
	#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
	#define MULTIBOOT_MEMORY_NVS                    4
	#define MULTIBOOT_MEMORY_BADRAM                 5	

	uint32_t type;
	uint32_t size;
	uint32_t entry_size;
	uint32_t entry_version;
	struct multiboot_mmap_entry entries[0];
};

struct multiboot_color
{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
};


struct multiboot_tag_framebuffer_common
{
	uint32_t type;
	uint32_t size;

	uint64_t framebuffer_addr;
	uint32_t framebuffer_pitch;
	uint32_t width;
	uint32_t height;
	uint8_t bpp;
	#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED	0
	#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB		1
	#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT	2
	uint8_t framebuffer_type;
	uint16_t reserved;
};

struct multiboot_tag_framebuffer
{
	struct multiboot_tag_framebuffer_common common;

	union
	{
		struct
		{
			uint16_t framebuffer_palette_num_colors;
			struct multiboot_color framebuffer_palette[0];
		};

		struct
		{
			uint8_t framebuffer_red_field_position;
			uint8_t framebuffer_red_mask_size;
			uint8_t framebuffer_green_field_position;
			uint8_t framebuffer_green_mask_size;
			uint8_t framebuffer_blue_field_position;
			uint8_t framebuffer_blue_mask_size;
		};
	};
};


void process_multiboot2_tags(unsigned long magic_number, unsigned long addr);

#endif /* MULTIBOOT2_H*/
