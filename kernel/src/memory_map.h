#pragma once

/* Kernel Memory Map */
#define KERNEL_VBASE 0xFFFFFFFF80000000ULL
#define USER_VBASE   0x0000000000000000ULL

/* Paging Constants */
#define KERNEL_PML4_INDEX 256
#define USER_PML4_INDEX   0

/* Access Flags */
#define PAGE_PRESENT (1 << 0)
#define PAGE_WRITABLE (1 << 1)
#define PAGE_USER (1 << 2)
#define PAGE_NX (1ULL << 63)
