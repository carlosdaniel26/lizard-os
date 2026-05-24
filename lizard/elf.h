#pragma once

#include <nolibc/types.h>

#define EI_NIDENT 16

/* e_ident[] indexes */
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6

/* Magic */
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

/* Class */
#define ELFCLASS64 2

/* Data encoding */
#define ELFDATA2LSB 1

/* File types */
#define ET_NONE 0
#define ET_REL  1
#define ET_EXEC 2
#define ET_DYN  3

/* Machine */
#define EM_X86_64 62

/* Program header types */
#define PT_NULL    0
#define PT_LOAD    1
#define PT_DYNAMIC 2
#define PT_INTERP  3
#define PT_NOTE    4
#define PT_PHDR    6

/* Segment flags */
#define PF_X 1
#define PF_W 2
#define PF_R 4

typedef struct {
    unsigned char e_ident[EI_NIDENT];

    u16 e_type;
    u16 e_machine;
    u32 e_version;

    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;

    u32 e_flags;

    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;

    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} elf64_ehdr_t;

typedef struct {
    u32 p_type;
    u32 p_flags;

    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;

    u64 p_filesz;
    u64 p_memsz;

    u64 p_align;
} elf64_phdr_t;

typedef struct {
    u32 sh_name;
    u32 sh_type;

    u64 sh_flags;
    u64 sh_addr;
    u64 sh_offset;
    u64 sh_size;

    u32 sh_link;
    u32 sh_info;

    u64 sh_addralign;
    u64 sh_entsize;
} elf64_shdr_t;