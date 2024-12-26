#ifndef MULTIBOOT2_H
#define MULTIBOOT2_H

#include <stdint.h>

// https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#Boot-information
#define MULTIBOOT2_MAGIC 0x36d76289


struct multiboot2_header {
    uint32_t magic;          // 0x36D76289
    uint32_t architecture;   // Arch (0 if x86)
    uint32_t header_length;  // length in bytes
    uint32_t checksum;
}
 __attribute__((packed));

#define MULTIBOOT_TAG_TYPE_END               0
#define MULTIBOOT_TAG_TYPE_CMDLINE           1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME  2
#define MULTIBOOT_TAG_TYPE_MODULE            3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO     4
#define MULTIBOOT_TAG_TYPE_BOOTDEV           5
#define MULTIBOOT_TAG_TYPE_MMAP              6
#define MULTIBOOT_TAG_TYPE_VBE               7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER       8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS      9
#define MULTIBOOT_TAG_TYPE_APM               10
#define MULTIBOOT_TAG_TYPE_EFI32             11
#define MULTIBOOT_TAG_TYPE_EFI64             12
#define MULTIBOOT_TAG_TYPE_SMBIOS            13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD          14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW          15
#define MULTIBOOT_TAG_TYPE_NETWORK           16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP          17
#define MULTIBOOT_TAG_TYPE_EFI_BS            18
#define MULTIBOOT_TAG_TYPE_EFI32_IH          19
#define MULTIBOOT_TAG_TYPE_EFI64_IH          20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR    21

struct multiboot2_tag {
    uint32_t type;
    uint32_t size; 
};

struct multiboot2_tag_cmdline {
    struct multiboot2_tag tag;
    char *string;
};

struct multiboot2_tag_bootloader_name {
    struct multiboot2_tag tag;
    char *name;
};

struct multiboot2_tag_module {
    struct multiboot2_tag tag;
    uint32_t mod_start;
    uint32_t mod_end;
    char *cmdline;
    uint32_t reserved;
};

struct multiboot_mmap_entry
{
    uint64_t addr;
    uint64_t len;
    #define MULTIBOOT_MEMORY_AVAILABLE              1
    #define MULTIBOOT_MEMORY_RESERVED               2
    #define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE       3
    #define MULTIBOOT_MEMORY_NVS                    4
    #define MULTIBOOT_MEMORY_BADRAM                 5
    uint32_t type;
    uint32_t zero;
};

struct multiboot2_tag_mmap {
    struct multiboot2_tag tag;
    uint32_t size;
    uint32_t version;
    struct multiboot_mmap_entry entries[0];
};

struct multiboot2_tag_vbe {
    struct multiboot2_tag tag;
    uint32_t mode;
    uint32_t width;
    uint16_t height;
    uint8_t bpp;            // bits per pixel
    uint8_t reserved[3];
};

struct multiboot2_tag_end {
    struct multiboot2_tag tag;
};

struct multiboot_info_t
{
  /*  Must be MULTIBOOT_MAGIC - see above. */
  uint32_t magic_number;
  uint32_t architecture;
  uint32_t header_length;

  /*  The above fields plus this one must equal 0 mod 2^32. */
  uint32_t checksum;
};

void process_multiboot2_tags(unsigned long magic_number,unsigned long addr);

#endif