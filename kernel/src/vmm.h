#ifndef VMM_H
#define VMM_H

#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

void vmm_init();
void *vmm_alloc_page();
int vmm_free_page(uintptr_t ptr);

#endif
