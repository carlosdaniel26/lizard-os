#define BIT_SET(bitmap, index)	(bitmap[(index) / 8] |= (1 << ((index) % 8)))
#define BIT_CLEAR(bitmap, index)  (bitmap[(index) / 8] &= ~(1 << ((index) % 8)))
#define BIT_TEST(bitmap, index)   (bitmap[(index) / 8] & (1 << ((index) % 8)))

#define align_ptr_down(ptr, align) (\
	(void *)((uintptr_t)(ptr) & ~(align - 1)))

#define align_ptr_up(ptr, align) (\
	(void *)((((uintptr_t)(ptr)) + (align - 1)) & ~(align - 1)))

#define align_down(num, align) (num - (num % align))
#define align_up(num, align) (num + (align - (num % align)))

int oct2bin(unsigned char *str, int size);