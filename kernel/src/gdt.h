#pragma once

#include <stdint.h>

typedef struct global_descriptor {
	uint16_t limit_low;	 /* -> 0-15 */
	uint16_t base_low;	 /* -> 0-15 */
	uint8_t base_middle; /* -> 16-23 */
	uint8_t access;		 /* -> 17 */
	uint8_t granularity; /* -> 16-19 */
	uint8_t base_high;	 /* -> 24-31 */
} __attribute__((packed)) global_descriptor;

typedef struct gdt_ptr {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed)) gdt_ptr;

global_descriptor create_gdt_gate(uint64_t base, uint64_t limit, uint8_t access,
								  uint8_t granularity);
void init_gdt();