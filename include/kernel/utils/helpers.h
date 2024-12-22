#ifndef ALIAS_h
#define ALIAS_h

#define BIT_SET(bitmap, index)    (bitmap[(index) / 8] |= (1 << ((index) % 8)))
#define BIT_CLEAR(bitmap, index)  (bitmap[(index) / 8] &= ~(1 << ((index) % 8)))
#define BIT_TEST(bitmap, index)   (bitmap[(index) / 8] & (1 << ((index) % 8)))

#endif