#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#include <multiboot2.h>

typedef struct PageDirectory {
	/* bits 0-11 */
	uint8_t present:      	1;	// is present in memory
	uint8_t read_write:    	1;	// write(1) read(0)
	uint8_t user:			1;	// acess by user mode
	uint8_t pwt:			1;	// write-through cache
	uint8_t pcd: 			1;	// cache disable
	uint8_t accessed:		1;	// directory acessed
	uint8_t avl:			1;	// available
	uint8_t page_size:		1;	// page size

	/* bits 8-11 */
	uint8_t	available:		4;	// available

	/* bits 12-31 */
	uint32_t address:		20;	// address

} PageDirectory;

typedef struct PageTable {
	
	/* bits 0-8 */
	uint8_t present:      	1;	// is present in memory on the moment
	uint8_t read_write:    	1;	// write(1) read(0) permissions
	uint8_t user:			1;	// acess by user (0) by all (1)
	uint8_t pwt:			1;	// disable(0)write-through caching enable (1)
	uint8_t pcd:			1;	// cache enable(1) cache disable (0)
	uint8_t accessed:		1;	// directory acessed
	uint8_t dirty:			1;	// clean (0) dirty(1)
	uint8_t pat:			1;	// Page Attribute Table'
	uint8_t global:			1;	// 'Global' tells the processor not to invalidate the TLB entry corresponding to the page upon a MOV to CR3 instruction.

	/* bits 9-11 */
	uint8_t	available:		4;	// available for use

	/* bits 12-31 */
	uint32_t address:		20;	// address
	
} PageTable;

void init_paging();

#endif