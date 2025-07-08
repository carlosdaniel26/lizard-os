#include <stdint.h>

#define BIT_SET(bitmap, index) (bitmap[(index) / 8] |= (1 << ((index) % 8)))
#define BIT_CLEAR(bitmap, index) (bitmap[(index) / 8] &= ~(1 << ((index) % 8)))
#define BIT_TEST(bitmap, index) (bitmap[(index) / 8] & (1 << ((index) % 8)))

#define align_ptr_down(ptr, align) ((void *)((uintptr_t)(ptr) & ~(align - 1)))

#define align_ptr_up(ptr, align) ((void *)((((uintptr_t)(ptr)) + (align - 1)) & ~(align - 1)))

#define align_down(num, align) (num - (num % align))
#define align_up(num, align) (num + (align - (num % align)))

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

char toupper(char c);

struct Uptime {
    uint8_t years;
    uint8_t months;
    uint8_t days;
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
};

void hlt();
int oct2bin(unsigned char *str, int size);
void save_boot_time();
struct Uptime calculate_uptime();