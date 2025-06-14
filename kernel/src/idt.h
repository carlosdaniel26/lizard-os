#define IDT_ENTRIES 256

typedef struct {
    uint16_t offset_low;     /* -> 0-15 */
    uint16_t selector;       /* -> 0-15 */
    uint8_t  ist;            /* -> 0-7 */
    uint8_t  type_attr;      /* -> 0-7 */
    uint16_t offset_mid;     /* -> 16-31 */
    uint32_t offset_high;    /* -> 32-63 */
    uint32_t zero;           /* -> 0-31 */
} __attribute__((packed)) idt_entry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_ptr;

void init_idt();