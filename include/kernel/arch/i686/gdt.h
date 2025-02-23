#ifdef TARGET_I686
#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/*
 * GDT Entry Structure
 *
 *  31							 16 15							 0
 * +-------------------------------+-------------------------------+
 * |		  Base 31:24		   |		  Base 23:16		   |
 * +-------------------------------+-------------------------------+
 * |   G   | D/B | L | AVL |  Limit 19:16  | A |  Limit 15:0	   |
 * +-------------------------------+-------------------------------+
 * |		   Base 15:0		   |			Limit 15:0		 |
 * +-------------------------------+-------------------------------+
 *
 * Base  - Base address of the segment
 * Limit - Limit of the segment
 * G	 - Granularity
 * D/B   - Size (0 for 16-bit, 1 for 32-bit)
 * L	 - 64-bit code segment (IA-32e mode only)
 * AVL   - Available for use by system software
 * A	 - Accessed bit
 */

typedef struct global_descriptor {
	uint16_t limit_low;
	uint16_t base_low;
	uint8_t  base_middle;
	uint8_t  access;
	uint8_t  granularity;
	uint8_t  base_high;
} __attribute__((packed)) global_descriptor;

typedef struct gdt_ptr {
	uint16_t limit;
	uint32_t base;
} __attribute__((packed)) gdt_ptr;

void init_gdt();
global_descriptor create_gdt_gate(uint32_t base, uint32_t limit, uint8_t access, uint8_t granularity);
void load_gdt();

#endif
#endif
