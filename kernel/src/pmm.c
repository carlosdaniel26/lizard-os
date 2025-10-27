#include <alias.h>
#include <framebuffer.h>
#include <helpers.h>
#include <limine.h>
#include <pmm.h>
#include <stddef.h>
#include <types.h>
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


u64 mem_ammount_b = 0;
u64 usable_memory = 0;
u8 *bitmap = NULL;
u64 total_blocks = 0;
u64 usable_blocks = 0;
u64 hhdm_offset = 0;

static inline void pmm_reserve_block(u64 i)
{
	BIT_SET(bitmap[i / 8], (i % 8));
}
static inline void pmm_unreserve_block(u64 i)
{
	BIT_CLEAR(bitmap[i / 8], (i % 8));
}
static inline int pmm_test_block(u64 i)
{
	return BIT_TEST(bitmap[i / 8], (i % 8));
}

void pmm_init()
{
	hhdm_offset = hhdm_request.response->offset;

	/* Count memory */
	for (u64 i = 0; i < memmap_request.response->entry_count; i++)
	{
		struct limine_memmap_entry *e = memmap_request.response->entries[i];
		mem_ammount_b += e->length;
	}

	total_blocks = mem_ammount_b / BLOCK_SIZE;

	/* Init memory as reserved */
	for (u64 i = 0; i < memmap_request.response->entry_count; i++)
	{
		struct limine_memmap_entry *e = memmap_request.response->entries[i];
		if (e->type == LIMINE_MEMMAP_USABLE && e->length >= (total_blocks / 8 + 1))
		{
			bitmap = (u8 *)ALIGN_UP(e->base + hhdm_offset, BLOCK_SIZE);
			memset(bitmap, 0xFF, total_blocks / 8 + 1);
			break;
		}
	}

	/* Free the usable blocks */
	for (u64 i = 0; i < memmap_request.response->entry_count; i++)
	{
		struct limine_memmap_entry *e = memmap_request.response->entries[i];

		if (e->type == LIMINE_MEMMAP_FRAMEBUFFER)
			framebuffer_length = e->length;
		if (e->type != LIMINE_MEMMAP_USABLE)
			continue;

		uintptr_t start = ALIGN_UP((uintptr_t)e->base, BLOCK_SIZE);
		uintptr_t end = align_down((uintptr_t)e->base + e->length, BLOCK_SIZE);

		for (uintptr_t addr = start; addr < end; addr += BLOCK_SIZE)
		{
			u64 bid = addr / BLOCK_SIZE;
			if (bid >= total_blocks)
				continue;
			pmm_unreserve_block(bid);
			usable_blocks++;
		}

		usable_memory = usable_blocks * BLOCK_SIZE;
	}

	/* Protect Bitmap Area*/
	uintptr_t phys_bitmap = (uintptr_t)(bitmap - hhdm_offset);
	u64 size = (total_blocks / 8) + 1;
	uintptr_t end = ALIGN_UP(phys_bitmap + size, BLOCK_SIZE);

	for (uintptr_t addr = align_down(phys_bitmap, BLOCK_SIZE); addr < end; addr += BLOCK_SIZE)
	{
		u64 bid = addr / BLOCK_SIZE;
		if (bid < total_blocks)
			pmm_reserve_block(bid);
	}

	debug_printf("PMM: Initialized. Total Memory: %u bytes, Usable Memory: %u bytes\n", mem_ammount_b,
				 usable_memory);
	// pmm_test_all();
}

void *pmm_alloc_block()
{
	for (u64 i = 0; i < total_blocks; i++)
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
	u64 block = ((u64)ptr) / BLOCK_SIZE;

	pmm_unreserve_block(block);
}

void pmm_free_block_row(void *ptr, u64 ammount)
{
	u64 start_block = ((u64)ptr) / BLOCK_SIZE;

	for (u64 i = start_block; i < start_block + ammount; i++)
	{
		pmm_unreserve_block(i);
	}
}

void *pmm_alloc_block_row(u64 ammount)
{
	if (ammount == 0)
		return NULL;

	void *base = 0x00;
	u64 base_block = 0;
	u64 free_in_row = 0;

	for (u64 i = 0; i < total_blocks; i++)
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
				for (u64 block = base_block; block < base_block + ammount; block++)
					pmm_reserve_block(block);

				return (void *)((u64)base);

			}
		}
		else
		{
			base = NULL;
			base_block = 0;
			free_in_row = 0;
		}
	}
	return NULL;
}

u64 pmm_free_block_count()
{
	u64 free = 0;

	for (block_id_t i = 0; i < total_blocks; i++)
	{
		if (! pmm_test_block(i))
		{
			free++;
		}
	}
	return free;
}

u64 pmm_used_block_count()
{
	return usable_blocks - pmm_free_block_count();
}

void pmm_test_all()
{
	kprintf("PMM: Testing all blocks...\n");
	u64 tested = 0;

	u64 i = 0;
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