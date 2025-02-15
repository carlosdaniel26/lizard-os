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

typedef uint8_t multiboot_uint8_t;

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
	uint32_t type;
	uint32_t size;
	uint32_t entry_size;
	uint32_t entry_version;
	struct multiboot_mmap_entry entries[0];
};

void process_multiboot2_tags(unsigned long magic_number, unsigned long addr);

#endif /* MULTIBOOT2_H*/