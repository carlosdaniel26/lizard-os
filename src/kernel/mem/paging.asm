section .bss
    align 4096
page_directory: resb 4096 ; Page directory (4 KiB aligned)
    align 4096
page_table:     resb 4096 ; Page table (4 KiB aligned)

section .text
global enable_paging
enable_paging:
    ; Zero out the page directory and page table manually
    xor eax, eax          ; Set EAX to 0
    mov ecx, 1024         ; Number of entries in the page directory

    ; Set up page table for 0x100000 to 0x128000
    lea edi, page_table
    mov ecx, (0x128000) / 4096 ; Number of pages
    
    mov eax, 0x0     ; First P_Adress to map
setup_page_table:
    mov ebx, eax          ; Save the address for calculations

    shr ebx, 12           ; Get the page number

    mov edx, eax          ; Save the address for entry
    or edx, 0x3           ; Mark as present and writable

    mov [edi + ebx * 4], edx   ; Write the entry to the page table
    add eax, 0x1000       ; Move to the next page
    loop setup_page_table

    ; Link page table in page directory
    mov eax, page_table
    or eax, 0x3           ; Mark as present and writable
    mov [page_directory], eax


load_registers:

    ; Load the page directory into CR3
    mov eax, page_directory
    mov cr3, eax
	

    ; Enable paging by setting PG bit in CR0
    mov eax, cr0
    or eax, 0x80000001    ; Set the paging bit
    mov cr0, eax

    ret