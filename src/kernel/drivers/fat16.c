#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <kernel/utils/helpers.h>
#include <kernel/drivers/fat16.h>
#include <kernel/drivers/ata.h>
#include <kernel/mem/kmalloc.h>

static int fat16_cmp_names(const char *fat_entry_name, const char *input_filename)
{
    const char *extension_position = strchr(input_filename, '.');
    size_t base_name_length;
    size_t extension_length;

    if (extension_position != NULL) 
    {
        base_name_length = (size_t)(extension_position - input_filename);
        extension_length = strlen(extension_position + 1);
    } 
    else 
    {
        base_name_length = strlen(input_filename);
        extension_length = 0;
    }

    /* Validate base name and extension lengths */
    if (base_name_length > FAT_NAME_LENGTH || extension_length > FAT_EXT_LENGTH)
        return 0;

    /* Compare base name characters */
    for (size_t index = 0; index < FAT_NAME_LENGTH; index++) 
    {
        char input_char;

        if (index < base_name_length)
            input_char = toupper(input_filename[index]);
        else
            input_char = FAT_PADDING_CHAR;

        char fat_char = toupper(fat_entry_name[index]);
        if (fat_char != input_char) 
            return 0;
    }

    /* Extension not found */
    if (extension_position == NULL)
        return 0;

    /* Compare extension characters */
    const char *input_extension = extension_position + 1;
    for (size_t index = 0; index < FAT_EXT_LENGTH; index++) 
    {
        char extension_char;

        if (index < extension_length)
            extension_char = toupper(input_extension[index]);
        else
            extension_char = FAT_PADDING_CHAR;

        char fat_char = toupper(fat_entry_name[FAT_NAME_LENGTH + index]);

        if (fat_char != extension_char)
            return 0;
    }

    return 1;
}

static int fat16_read_boot_sector(ATADevice *device, FAT16BPB *bios_parameter_block)
{
    if (atapio_read_sector(device, 0, (char*)bios_parameter_block) != 0)
        return -1;

    return 0;
}

static char *fat16_read_root_directory(ATADevice *device, const FAT16BPB *bios_parameter_block)
{
    uint32_t total_directory_bytes = bios_parameter_block->root_entry_count * FAT_DIR_ENTRY_SIZE;

    uint32_t adjusted_total_bytes = total_directory_bytes 
                                  + bios_parameter_block->bytes_per_sector 
                                  - 1;

    uint32_t bytes_per_sec = bios_parameter_block->bytes_per_sector;

    uint32_t root_directory_sectors = adjusted_total_bytes / bytes_per_sec;

    size_t root_size = root_directory_sectors * bios_parameter_block->bytes_per_sector;
    
    char *root_directory = kmalloc(root_size);

    if (root_directory == NULL)
        return NULL;

    uint32_t reserved_sectors = bios_parameter_block->reserved_sectors;
    uint32_t total_fat_sectors = bios_parameter_block->num_fats 
                               * bios_parameter_block->sectors_per_fat;
                               
    uint32_t root_start_sector = reserved_sectors + total_fat_sectors;

    for (uint32_t sector_index = 0; sector_index < root_directory_sectors; sector_index++)
    {
        char *sector_buffer = root_directory + sector_index * bios_parameter_block->bytes_per_sector;

        if (atapio_read_sector(device, root_start_sector + sector_index, sector_buffer) != 0) 
        {
            kfree(root_directory);
            return NULL;
        }
    }

    return root_directory;
}

static const FAT16Dir *fat16_find_file_entry(const char *filename, 
                                             const char *root_directory, 
                                             uint32_t total_entries)
{
    for (uint32_t entry_index = 0; entry_index < total_entries; entry_index++) 
    {
        const FAT16Dir *directory_entry = (const FAT16Dir*)
            (root_directory + entry_index * FAT_DIR_ENTRY_SIZE);

        if (directory_entry->name[0] == 0x00)
            break;

        if (directory_entry->attributes.volume_id || directory_entry->attributes.hidden)
            continue;

        if (fat16_cmp_names(directory_entry->name, filename))
            return directory_entry;
    }

    return NULL;
}

static uint16_t *fat16_read_fat(ATADevice *device, const FAT16BPB *bios_parameter_block)
{
    size_t fat_size = bios_parameter_block->sectors_per_fat * bios_parameter_block->bytes_per_sector;
    uint16_t *file_allocation_table = kmalloc(fat_size);

    if (file_allocation_table == NULL)
        return NULL;

    uint32_t fat_start_sector = bios_parameter_block->reserved_sectors;
    for (uint32_t sector_index = 0; sector_index < bios_parameter_block->sectors_per_fat; sector_index++) 
    {
        char *sector_buffer = (char *)file_allocation_table + sector_index * bios_parameter_block->bytes_per_sector;
        
        if (atapio_read_sector(device, fat_start_sector + sector_index, sector_buffer) != 0) 
        {
            kfree(file_allocation_table);
            return NULL;
        }
    }

    return file_allocation_table;
}

/* temp */
static void fat16_print_file_data(ATADevice *dev, const FAT16BPB *bpb,
                                 uint16_t start_cluster, uint32_t file_size,
                                 const uint16_t *fat)
{
    char sector_buffer[bpb->bytes_per_sector];
    uint32_t bytes_remaining = file_size;
    uint16_t current_cluster = start_cluster;
    const uint32_t first_data_sector = bpb->reserved_sectors 
                                     + bpb->num_fats * bpb->sectors_per_fat 
                                     + ((bpb->root_entry_count * 32) 
                                     + bpb->bytes_per_sector - 1) 
                                     / bpb->bytes_per_sector;

    while (current_cluster >= FAT16_CLUSTER_MIN 
           && current_cluster < FAT16_EOC_MIN
           && bytes_remaining > 0) 
    {
        
        uint32_t sector = first_data_sector 
                        + (current_cluster - 2) * bpb->sectors_per_cluster;

        for (uint32_t sec = 0; sec < bpb->sectors_per_cluster; sec++) 
        {
            if (bytes_remaining == 0) break;
            
            if (atapio_read_sector(dev, sector + sec, sector_buffer) != 0)
                return;

            uint32_t bytes_to_process = MIN(bytes_remaining, 
                                           bpb->bytes_per_sector);
            for (uint32_t i = 0; i < bytes_to_process; i++)
                putchar(sector_buffer[i]);
            
            bytes_remaining -= bytes_to_process;
        }

        current_cluster = fat[current_cluster];
    }
}

int fat16_print_file(const char *filename)
{
    ATADevice *dev = ata_get(PRIMARY);
    if (! dev) return -1;

    /* Read boot sector */
    FAT16BPB bpb;
    if (fat16_read_boot_sector(dev, &bpb) != 0)
        return -1;

    /* Read root directory */
    char *root_dir = fat16_read_root_directory(dev, &bpb);
    if (!root_dir) return -1;

    /* Find file entry */
    const FAT16Dir *entry = fat16_find_file_entry(
        filename, root_dir, bpb.root_entry_count
    );
    if (!entry) {
        kfree(root_dir);
        return -1;
    }

    /* Read FAT */
    uint16_t *fat = fat16_read_fat(dev, &bpb);
    if (!fat) {
        kfree(root_dir);
        return -1;
    }

    /* Print file contents */
    fat16_print_file_data(dev, &bpb, entry->first_cluster_low, 
                         entry->file_size, fat);

    kfree(root_dir);
    kfree(fat);

    return 0;
}

void fat16_test_read_bpb_and_file()
{
    fat16_print_file("carlos.txt");
}