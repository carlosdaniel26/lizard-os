#define PAGE_PRESENT 0x1
#define PAGE_WRITABLE 0x2
#define PAGE_USER 0x4

#define PAGE_SIZE 4096

void vmm_init();
void vmm_load_pml4();
void vmm_map_kernel();
void vmm_map_framebuffer();
void vmm_map_stack();