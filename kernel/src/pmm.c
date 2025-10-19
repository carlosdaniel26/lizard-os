#include <alias.h>
#include <framebuffer.h>
#include <helpers.h>
#include <limine.h>
#include <pmm.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <tty.h>

#define ALIGN_UP(x, a) (((x) + ((a)-1)) & ~((a)-1))
#define BLOCK_SIZE 4096

__attribute__((used, section(".limine_requests"))) static volatile struct limine_memmap_request
    memmap_request = {.id = LIMINE_MEMMAP_REQUEST, .revision = 0};

__attribute__((
    used, section(".limine_requests"))) static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST, .revision = 0};


uint64_t mem_ammount_b = 0;
uint64_t mem_usable_ammount_b = 0;
uint64_t mem_unusable_ammount_b = 0;
uint8_t *bitmap = NULL;
uint32_t total_blocks = 0;
uint32_t usable_blocks = 0;
uint32_t unusable_blocks = 0;
uint64_t hhdm_offset = 0;

static inline void pmm_reserve_block(uint64_t i)
{
    BIT_SET(bitmap[i / 8], (i % 8));
}
static inline void pmm_unreserve_block(uint64_t i)
{
    BIT_CLEAR(bitmap[i / 8], (i % 8));
}
static inline int pmm_test_block(uint64_t i)
{
    return BIT_TEST(bitmap[i / 8], (i % 8));
}

void pmm_init()
{
    hhdm_offset = hhdm_request.response->offset;

    /* Count memory */
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        mem_ammount_b += e->length;

        if (e->type == LIMINE_MEMMAP_USABLE)
            mem_usable_ammount_b += e->length;
    }

    mem_unusable_ammount_b = mem_ammount_b - mem_usable_ammount_b;

    total_blocks = mem_ammount_b / BLOCK_SIZE;
    usable_blocks = mem_usable_ammount_b / BLOCK_SIZE;
    unusable_blocks = (mem_unusable_ammount_b + 1) / BLOCK_SIZE; /* Round up with the +1*/

    uint64_t bitmap_size = (total_blocks + 7) / 8;  /* Ceiling division */

    /* Find space for bitmap & Init memory as reserved */
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->length >= bitmap_size)
        {
            bitmap = (uint8_t *)(e->base + hhdm_offset);
            memset(bitmap, 0xFF, bitmap_size);

            /* As bitmap area will be fixed i'll count it as an unusable area */
            uint64_t bitmap_blocks = (bitmap_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
            usable_blocks -= bitmap_blocks;
            unusable_blocks += bitmap_blocks;

            mem_unusable_ammount_b += bitmap_size;
            mem_usable_ammount_b += bitmap_size;

            break;
        }
    }

    if (!bitmap) 
    {
        kprintf("PMM: ERROR - No suitable space for bitmap found!\n");
        return;
    }

    /* Free the usable blocks */
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];

        if (e->type == LIMINE_MEMMAP_FRAMEBUFFER)
            framebuffer_length = e->length;
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;

        uintptr_t start = ALIGN_UP((uintptr_t)e->base, BLOCK_SIZE);
        uintptr_t end = (uintptr_t)(e->base + e->length);

        for (uintptr_t addr = start; addr < end; addr += BLOCK_SIZE)
        {
            uint64_t bid = addr / BLOCK_SIZE;
            if (bid >= total_blocks)
                continue;
            pmm_unreserve_block(bid);
        }
    }

    /* Protect Bitmap Area*/
    uintptr_t phys_bitmap = (uintptr_t)(bitmap - hhdm_offset);
    uintptr_t start_bitmap = ALIGN_UP(phys_bitmap, BLOCK_SIZE);
    uintptr_t end_bitmap = ALIGN_UP(phys_bitmap + bitmap_size, BLOCK_SIZE);

    for (uintptr_t addr = start_bitmap; addr < end_bitmap; addr += BLOCK_SIZE)
    {
        uint64_t bid = addr / BLOCK_SIZE;
        if (bid < total_blocks)
            pmm_reserve_block(bid);
    }
}

void *pmm_alloc_block()
{
    for (uint64_t i = 0; i < total_blocks; i++)
    {
        if (!pmm_test_block(i))
        {
            pmm_reserve_block(i);
            return (void *)(i * BLOCK_SIZE);
        }
    }
    return NULL;
}

void pmm_free_block(void *ptr)
{
    uint64_t block = ((uint64_t)ptr - hhdm_offset) / BLOCK_SIZE;

    pmm_unreserve_block(block);
}

void *pmm_alloc_block_row(uint64_t ammount)
{
    if (ammount == 0)
        return NULL;

    for (uint64_t i = 0; i < total_blocks; i++)
    {
        uint8_t found = 1;

        for (uint64_t j = 0; j < ammount; j++)
        {
            if (pmm_test_block(i + j))
            {
                found = 0;
                i += j;
                break;
            }
        }

        if (found)
        {
            for (uint64_t j = 0; j < ammount; j++)
                pmm_reserve_block(i + j);
        }

        return (void *)(i * BLOCK_SIZE);

    }
    return NULL;
}

uint64_t pmm_free_block_count()
{
    uint64_t free = 0;

    for (block_id_t i = 0; i < total_blocks; i++)
    {
        if (! pmm_test_block(i))
        {
            free++;
        }
    }
    return free;
}

void pmm_test_all()
{
    kprintf("PMM: Testing all blocks...\n");
    uint64_t tested = 0;

    uint64_t i = 0;
    while (1)
    {
        void *ptr = pmm_alloc_block();
        if (!ptr)
            break;

        memset(ptr, 0xAB, BLOCK_SIZE);
        kprintf("  tested block $%x at %x\n", i, ptr);
        i++;
    }

    tty_clean();
    kprintf("COUNTED %u USABLE BLOCKS\n", i);

    kprintf("PMM: Finished. Total blocks tested: %u\n", tested);
}