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
uint8_t *bitmap = NULL;
uint64_t total_blocks = 0;
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
        if (e->type == LIMINE_MEMMAP_USABLE)
            mem_ammount_b += e->length;
    }

    total_blocks = mem_ammount_b / BLOCK_SIZE;

    /* Init memory as reserved */
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE && e->length >= (total_blocks / 8 + 1))
        {
            bitmap = (uint8_t *)(e->base + hhdm_offset);
            memset(bitmap, 0xFF, total_blocks / 8 + 1);
            break;
        }
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
    uint64_t size = (total_blocks / 8) + 1;
    uintptr_t end = ALIGN_UP(phys_bitmap + size, BLOCK_SIZE);

    for (uintptr_t addr = ALIGN_UP(phys_bitmap, BLOCK_SIZE); addr < end; addr += BLOCK_SIZE)
    {
        uint64_t bid = addr / BLOCK_SIZE;
        if (bid < total_blocks)
            pmm_reserve_block(bid);
    }

    // pmm_test_all();
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

    void *base = 0x00;
    uint64_t base_block = 0;
    uint64_t free_in_row = 0;

    for (uint64_t i = 0; i < total_blocks; i++)
    {
        if (!pmm_test_block(i))
        {
            if (base == NULL)
            {
                base = (void *)(i * BLOCK_SIZE);
                base_block = i;
            }

            free_in_row++;

            if (free_in_row == ammount)
            {
                /* Reserve each block */
                for (uint64_t block = base_block; block <= base_block; block++)
                {
                    pmm_reserve_block(block);
                }

                return (void *)((uint64_t)base);
            }
        }
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