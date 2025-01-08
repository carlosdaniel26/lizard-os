#include <kernel/mem/pmm.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#define PAGE_SIZE 4096

uint32_t *page_directory;
uint32_t *page_table;

extern void setup_paging_asm(uint32_t *page_directory, uint32_t *page_table);

void setup_paging()
{
    // Aloca e configura o diretório de páginas
    page_directory = (uint32_t *)pmm_alloc_block();
    if (!page_directory)
    {
        printf("Falha ao alocar o diretório de páginas\n");
        return;
    }

    // Aloca e configura a tabela de páginas
    page_table = (uint32_t *)pmm_alloc_block();
    if (!page_table)
    {
        printf("Falha ao alocar a tabela de páginas\n");
        return;
    }

    // Configure a tabela de páginas (mapeamento de identidade para os primeiros 4MB)
    for (int i = 0; i < 1024; i++)
    {
        page_table[i] = (i * PAGE_SIZE) | 0b11; // Define os bits de presente e escrita
    }

    // Chama a função em assembly para configurar o diretório de páginas e ativar a paginação
}

typedef uint32_t v_addr;
void idpaging(uint32_t *first_pte, v_addr from, int size)
{
    for (; size > 0; from += PAGE_SIZE, size -= PAGE_SIZE, first_pte++)
    {
        *first_pte = from | 1; // página presente
    }
}