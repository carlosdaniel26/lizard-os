#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <limine.h>
#include <tty.h>

#define ALIGN_UP(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define BLOCK_SIZE 4096

__attribute__((used, section(".limine_requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST, .revision = 0
};

__attribute__((used, section(".limine_requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0
};

uint64_t mem_ammount_b = 0;
uint8_t *bitmap = NULL;
uint64_t total_blocks = 0;
uint64_t hhdm_offset = 0;

static inline void set_bit(uint64_t i) { bitmap[i / 8] |= (1 << (i % 8)); }
static inline void clear_bit(uint64_t i) { bitmap[i / 8] &= ~(1 << (i % 8)); }
static inline int test_bit(uint64_t i) { return (bitmap[i / 8] >> (i % 8)) & 1; }

void pmm_init() 
{
    hhdm_offset = hhdm_request.response->offset;

    uint64_t mem_bytes = 0;
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) 
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE)
            mem_bytes += e->length;
    }

    total_blocks = mem_bytes / BLOCK_SIZE;

    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->length >= (total_blocks / 8 + 1)) 
        {
            bitmap = (uint8_t*)(e->base + hhdm_offset);
            memset(bitmap, 0xFF, total_blocks / 8 + 1);
            break;
        }
    }

    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) 
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE) continue;

        uintptr_t start = ALIGN_UP((uintptr_t)e->base, BLOCK_SIZE);
        uintptr_t end = (uintptr_t)(e->base + e->length);

        for (uintptr_t addr = start; addr < end; addr += BLOCK_SIZE) 
        {
            uint64_t bid = addr / BLOCK_SIZE;
            if (bid >= total_blocks) continue;
            clear_bit(bid);
        }
    }

    /* Protect Bitmap Area*/
	uintptr_t phys_bitmap = (uintptr_t)(bitmap - hhdm_offset);
	uint64_t size = (total_blocks / 8) + 1;
	uintptr_t end = ALIGN_UP(phys_bitmap + size, BLOCK_SIZE);

	for (uintptr_t addr = ALIGN_UP(phys_bitmap, BLOCK_SIZE); addr < end; addr += BLOCK_SIZE)
	{
	    uint64_t bid = addr / BLOCK_SIZE;
	    if (bid < total_blocks)
	        set_bit(bid);
	}

    set_bit(0);
    //pmm_test_all();
}

void *pmm_alloc_block()
{
    for (uint64_t i = 0; i < total_blocks; i++) 
    {
        if (!test_bit(i))
        {
            set_bit(i);
            return (void *)(i * BLOCK_SIZE + hhdm_offset);
        }
    }
    return NULL;
}

void pmm_test_all()
{
    kprintf("PMM: Testing all blocks...\n");
    uint64_t tested = 0;

    uint64_t i = 0;
    while (1) 
    {
        void *ptr = pmm_alloc_block();
        if (!ptr) break;

        memset(ptr, 0xAB, BLOCK_SIZE);
        kprintf("  tested block $%x at %x\n", i, ptr);
        i++;
    }

    tty_clean();
    kprintf("COUNTED %u USABLE BLOCKS\n", i);

    kprintf("PMM: Finished. Total blocks tested: %u\n", tested);
}