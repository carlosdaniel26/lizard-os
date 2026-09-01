#pragma once

#include <nolibc/types.h>

/*
 * Hand-off contract between the UEFI loader (boot/) and the kernel.
 *
 * The loader loads the kernel ELF into physical RAM, fills a struct boot_info,
 * and jumps to the kernel ELF entry point (_start, see lizard/head.S) with a
 * pointer to it in RDI. The kernel is still running on the firmware's identity
 * map at that point; head.S builds the real page tables and jumps to the
 * higher half. Every pointer in here is a physical / identity-mapped address.
 */

#define BOOT_INFO_MAGIC 0x4C495A424F4F5431ULL /* "LIZBOOT1" */

/* Suggested higher-half direct-map base; the real value travels in hhdm_base. */
#define BOOT_HHDM_BASE 0xFFFF800000000000ULL

enum bi_mmap_type {
    BI_USABLE = 1,   /* free RAM the kernel may use                       */
    BI_RESERVED,     /* firmware / MMIO / anything not to be touched      */
    BI_ACPI_RECLAIM, /* ACPI tables, reclaimable once parsed              */
    BI_ACPI_NVS,     /* ACPI non-volatile storage, keep reserved          */
    BI_LOADER,       /* loader image + boot_info + page-table scratch     */
    BI_BAD,          /* known-bad RAM                                     */
};

struct bi_mmap_entry {
    u64 base;
    u64 len;
    u32 type; /* enum bi_mmap_type */
    u32 _pad;
};

struct boot_info {
    u64 magic;            /* BOOT_INFO_MAGIC                              */
    u64 hhdm_base;        /* higher-half direct-map virtual base          */
    u64 highest_addr;     /* one past the highest usable physical byte    */
    u64 kernel_phys_base; /* where the PT_LOAD segments were copied (2MiB aligned) */
    u64 kernel_virt_base; /* min p_vaddr, i.e. 0xffffffff80000000        */
    u64 kernel_size;      /* span of the PT_LOAD segments, page-rounded   */
    u64 scratch_phys_base; /* zeroed region head.S carves page tables from */
    u64 scratch_size;
    u64 mmap_count;
    struct bi_mmap_entry *mmap; /* physical pointer to mmap_count entries */
    char cmdline[256];

    /* Linear framebuffer from GOP; fb_base == 0 means none (serial only). */
    u64 fb_base;   /* physical address of the framebuffer                 */
    u64 fb_size;   /* bytes                                              */
    u32 fb_width;  /* pixels                                             */
    u32 fb_height; /* pixels                                             */
    u32 fb_pitch;  /* bytes per scanline                                 */
    u32 fb_bpp;    /* bits per pixel (32)                                */
    u32 fb_format; /* enum bi_fb_format                                  */
    u32 _fb_pad;
};

enum bi_fb_format {
    BI_FB_NONE = 0,
    BI_FB_BGRX = 1, /* byte order B,G,R,x  (QEMU/Limine default)          */
    BI_FB_RGBX = 2, /* byte order R,G,B,x                                 */
};

/* Set by head.S (_start_high) before it calls kmain(); initially points at the
 * loader-owned struct in low physical RAM. */
extern struct boot_info *boot_info_ptr;

/*
 * Copy the loader's boot_info (and its mmap array) into kernel-resident storage
 * and repoint boot_info_ptr at the copy. Must run from kmain() before
 * kernel_bootstrap(), while head.S's identity map is still active - after
 * vmm_init() switches page tables the loader's physical pointers are unmapped.
 */
void boot_info_relocate(void);
