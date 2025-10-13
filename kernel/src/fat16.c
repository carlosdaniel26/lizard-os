#include <ata.h>
#include <fat16.h>
#include <helpers.h>
#include <stdio.h>
#include <string.h>
#include <kmalloc.h>
#include <stdbool.h>

/* ===== CONSTANTS ===== */
#define FAT16_FAT_ENTRY_SIZE 2
#define FAT16_DIR_ENTRY_SIZE 32
#define FILE_NAME_SIZE 11

#define FAT16_DIR_ENTRY_DELETED 0xE5
#define FAT16_DIR_ENTRY_END 0x00

/* Attributes */
#define FAT16_ATTR_READ_ONLY 0x01
#define FAT16_ATTR_HIDDEN 0x02
#define FAT16_ATTR_SYSTEM 0x04
#define FAT16_ATTR_VOLUME_ID 0x08
#define FAT16_ATTR_DIRECTORY 0x10
#define FAT16_ATTR_ARCHIVE 0x20

/* Cluster values */
#define FAT16_FREE_CLUSTER      0x0000
#define FAT16_RESERVED_CLUSTER  0x0001
#define FAT16_BAD_CLUSTER       0xFFF7
#define FAT16_EOC         		0xFFF8

#define DIV_ROUND_UP(x, y) (((x) + (y)-1) / (y))

/* ===== INTERNAL HELPERS ===== */

static inline bool is_valid_entry(const Fat16Directory *entry) 
{
    return (entry->name[0] != FAT16_DIR_ENTRY_DELETED &&
            entry->name[0] != FAT16_DIR_ENTRY_END &&
            entry->name[0] != 0x00 &&
            entry->name[0] != 0xAC &&
            entry->file_size_bytes != 0xFFFFFFFF &&
            entry->attributes != 0xFF);
}

static uint32_t cluster_to_lba(Fat16 *fs, uint16_t cluster) 
{
    return  ((cluster - 2) * fs->header.sectors_per_cluster)
           + fs->data_region_lba;
}

static int read_fat_entry(Fat16 *fs, uint16_t cluster, uint16_t *next) 
{
    uint32_t fat_offset = cluster * FAT16_FAT_ENTRY_SIZE;
    uint32_t fat_sector = fs->fat_start_lba + (fat_offset / fs->header.bytes_per_sector);
    uint32_t entry_offset = fat_offset % fs->header.bytes_per_sector;

    char sector[512];
    if (atapio_read_sector(fs->disk, fat_sector, sector) != 0)
        return -1;

    memcpy(next, sector + entry_offset, sizeof(uint16_t));
    return 0;
}

static uint16_t get_next_cluster(Fat16 *fs, uint16_t cluster) 
{
    uint16_t next = FAT16_EOC;
    read_fat_entry(fs, cluster, &next);
    return next;
}

static void convert_name_to_83(const char *normal_name, char *out_83_name) 
{	
	/* Init */
	memset(out_83_name, ' ', FILE_NAME_SIZE);
	
	/* Get data */
	const char *dot = strchr(normal_name, '.');
	size_t name_len;

	if (dot)
		name_len = (size_t)(dot - normal_name);
	else
		name_len = strlen(normal_name);
	
	/* truncate */
	if (name_len > 8) name_len = 8;

	/* copy actual name */
	memcpy(out_83_name, normal_name, name_len);

	/* If has extension, copy it */
	if (dot)
	{
		/* Get the extension len ex: file.[txt] the "txt" len is 3 */
		size_t ext_len = strlen(dot + 1);

		/* Truncate */
		if (ext_len > 3) ext_len = 3;

		/*The extension is fixed position, so copy the text after the dot to the end of the 8.3 name*/
		memcpy(out_83_name + 8, dot + 1, ext_len);
	}

	/* FAT 83 names are always upper case, then convert */
	for (int i = 0; i < FILE_NAME_SIZE; i++)
		out_83_name[i] = toupper((unsigned char)out_83_name[i]);
}

static int compare_filenames(const char *filename, const Fat16Directory *entry)
{
    char formated[FILE_NAME_SIZE];

	convert_name_to_83(filename, formated);

	return (memcmp(formated, entry->name, FILE_NAME_SIZE) == 0);
}

int fat16_read_file(Fat16* fs, Fat16Directory* entry, char* buffer)
{
    if (!fs || !entry || !buffer) {
        return -1;
    }

    uint32_t file_size = entry->file_size_bytes;
    uint16_t current_cluster = entry->first_cluster_low;
    uint32_t bytes_read = 0;
    
    uint8_t sectors_per_cluster = fs->header.sectors_per_cluster;
    uint32_t bytes_per_cluster = sectors_per_cluster * 512;
    
    uint32_t data_start_lba = fs->root_dir_lba + 
                             (fs->header.root_entry_count * 32 + 511) / 512;

    char sector_buffer[512];
    
    kprintf("Reading file: clusters=%u, size=%u bytes\n", 
            current_cluster, file_size);
    
    while (current_cluster >= 0x0002 && current_cluster <= 0xFFEF && bytes_read < file_size)
    {
        uint32_t cluster_lba = data_start_lba + 
                              (current_cluster - 2) * sectors_per_cluster;
        
        kprintf("Reading cluster %u at LBA %u\n", current_cluster, cluster_lba);
        
        for (int i = 0; i < sectors_per_cluster && bytes_read < file_size; i++) 
        {
            if (atapio_read_sector(fs->disk, cluster_lba + i, sector_buffer) != 0) {
                kprintf("Error reading sector %u\n", cluster_lba + i);
                return -1;
            }
            
            uint32_t bytes_to_copy = 512;

            if (bytes_read + bytes_to_copy > file_size) 
                bytes_to_copy = file_size - bytes_read;
            
            memcpy(buffer + bytes_read, sector_buffer, bytes_to_copy);
            bytes_read += bytes_to_copy;
            
            kprintf("Read %u bytes from sector %u (total: %u/%u)\n", 
                   bytes_to_copy, cluster_lba + i, bytes_read, file_size);
        }
        
        current_cluster = get_next_cluster(fs, current_cluster);
        
        if (current_cluster == 0xFFFF) {
            kprintf("End of file cluster chain\n");
            break;
        }
    }
    
    kprintf("File read completed: %u bytes read\n", bytes_read);
    return 0;
}

int find_in_root(Fat16 *fs, const char *filename, Fat16Directory *out)
{
	char buffer[512];
	char formated_filename[FILE_NAME_SIZE];
	convert_name_to_83(filename, formated_filename);

	uint32_t root_dir_entries = fs->header.root_entry_count;
	uint32_t root_dir_size = root_dir_entries * FAT16_DIR_ENTRY_SIZE;
	uint32_t root_dir_sectors = DIV_ROUND_UP(root_dir_size, fs->header.bytes_per_sector);
	

	for (uint32_t sector = 0; sector < root_dir_sectors; sector++) 
	{
		if (atapio_read_sector(fs->disk, fs->root_dir_lba + sector, buffer) != 0)
			return -1;

		for (uint16_t i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++) 
		{
			Fat16Directory entry;
			memcpy(&entry, buffer + (i * FAT16_DIR_ENTRY_SIZE), sizeof(Fat16Directory));

			if (! is_valid_entry(&entry))
				continue;

			if (compare_filenames(formated_filename, &entry) == 0)
			{
				memcpy(out, &entry, sizeof(Fat16Directory));

				return 0; /* Found */
			}
		}
	}

	return -1; /* Not found */
}

int find_in_dir(Fat16 *fs, uint16_t start_cluster, const char *filename, Fat16Directory *out)
{
	char formated[FILE_NAME_SIZE];
	convert_name_to_83(filename, formated);
	uint16_t current_cluster = start_cluster;
	char buffer[512];

	do {
		uint32_t lba = cluster_to_lba(fs, current_cluster);

		for (uint8_t sector = 0; sector < fs->header.sectors_per_cluster; sector++) 
		{
			if (atapio_read_sector(fs->disk, lba + sector, buffer) != 0)
				return -1;

			for (uint16_t i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++) {
				Fat16Directory entry;
				memcpy(&entry, buffer + (i * FAT16_DIR_ENTRY_SIZE), sizeof(Fat16Directory));

				if (is_valid_entry(&entry) && compare_filenames(formated, &entry) == 0) {
					memcpy(out, &entry, sizeof(Fat16Directory));
					return 0; /* Found */
				}
			}
		}
		
		current_cluster = get_next_cluster(fs, current_cluster);
	} while (current_cluster < FAT16_EOC);

	return -1; /* Not found */
}

int find_in_path(Fat16 *fs, const char *path, Fat16Directory *out)
{
    Fat16Directory current_entry;
    current_entry.first_cluster_low = 0; /* root dir */

    /** Count slashes in path */
    int count_slash = 0;
    for (const char* p = path; *p; p++) {
        if (*p == '/') count_slash++;
    }

    int N = count_slash + 1;
    
    /** Allocate token matrix: N tokens x 64 characters each */
    char **tokens = (char **)kmalloc(N * sizeof(char *));
    if (!tokens) {
        kprintf("Error: Failed to allocate token array\n");
        return -1;
    }
    
    for (int i = 0; i < N; i++) {
        tokens[i] = (char *)kmalloc(64 * sizeof(char));
        if (!tokens[i]) {
            kprintf("Error: Failed to allocate token %u\n", i);
            /** Cleanup previously allocated tokens */
            for (int j = 0; j < i; j++) {
                kfree(tokens[j]);
            }
            kfree(tokens);
            return -1;
        }
        tokens[i][0] = '\0'; /** Initialize as empty string */
    }

    /** Tokenize path */
    const char *p = path;
    int token_index = 0;
    int char_index = 0;

    /** Skip leading slashes */
    while (*p == '/')
        p++;

    while (*p && token_index < N) {
        char_index = 0;

        /** Extract token until next slash or end of string */
        while (*p && *p != '/' && char_index < 63) {
            tokens[token_index][char_index] = *p;
            char_index++;
            p++;
        }

        /** Terminate token string */
        tokens[token_index][char_index] = '\0';
        token_index++;

        /** Skip consecutive slashes */
        while (*p == '/')
            p++;
    }

    /** Print tokens with count */
    kprintf("=== TOKENS FOUND ===\n");
    kprintf("Total tokens: %u\n", token_index);
    kprintf("Original path: '%s'\n", path);
    for (int i = 0; i < token_index; i++) {
        kprintf("Token [%u/%u]: '%s'\n", i + 1, token_index, tokens[i]);
    }
    kprintf("====================\n");

    /** Search for each token in filesystem */
    int is_root = 1;

    for (int i = 0; i < token_index; i++) {
        /** Skip empty tokens */
        if (tokens[i][0] == '\0')
            continue;

        Fat16Directory entry;
        int result;

        if (is_root) 
		{
            kprintf("Searching in ROOT: %s\n", tokens[i]);
            result = find_in_root(fs, tokens[i], &entry);
            is_root = 0;
        } else {
            kprintf("Searching in cluster %u: %s\n",
                    current_entry.first_cluster_low, tokens[i]);
            result = find_in_dir(fs, current_entry.first_cluster_low, tokens[i], &entry);
        }

        if (result != 0) 
		{
            kprintf("  -> '%s' not found\n", tokens[i]);
            /** Cleanup tokens before returning */
            for (int j = 0; j < N; j++) {
                kfree(tokens[j]);
            }
            kfree(tokens);
            return -1;
        }

        memcpy(&current_entry, &entry, sizeof(Fat16Directory));
    }

    /** Cleanup tokens */
    for (int i = 0; i < N; i++) {
        kfree(tokens[i]);
    }
    kfree(tokens);

    memcpy(out, &current_entry, sizeof(Fat16Directory));
    return 0;
}

void test_vfs()
{
    ATADevice *dev = ata_get(0);
    
    if (!dev || !dev->present)
    {
        kprintf("No ATA device found\n");
        return;
    }

    /* Read FAT16 Reader */
    Fat16 fs;
    fs.disk = dev;
    char buffer[512] = {0};
    if (atapio_read_sector(dev, 0, &buffer) != 0) {
        kprintf("Error reading boot sector\n");
        return;
    }
    memcpy(&fs.header, buffer, sizeof(FatHeader));
    
    /* Calculate root directory position */
    fs.fat_start_lba = fs.header.reserved_sector_count;
    fs.root_dir_lba = fs.fat_start_lba + (fs.header.num_fats * fs.header.fat_size_16);

    kprintf("FAT16 Info: Root LBA=%u, FAT LBA=%u\n", fs.root_dir_lba, fs.fat_start_lba);

    Fat16Directory entry;

    char file_name[64] = "/CARLOS.TXT";
    
    /* Test simple file first */
    kprintf("\n=== Testing %s ===\n", file_name);
    if(find_in_path(&fs, file_name, &entry) != 0)
    {
        kprintf("File %s not found\n", file_name);
    }
    else
    {
        kprintf("SUCCESS: FILE NAME: %s, SIZE: %u bytes, CLUSTER: %u\n", 
                entry.name, entry.file_size_bytes, entry.first_cluster_low);
        
        char *file_content = (char*)kmalloc(entry.file_size_bytes + 1);
        if (!file_content) {
            kprintf("ERROR: Failed to allocate memory for file content\n");
            return;
        }
        
        if (fat16_read_file(&fs, &entry, file_content) == 0) 
        {
            file_content[entry.file_size_bytes] = '\0';
            
            kprintf("\n=== FILE CONTENT ===\n");
            kprintf("%s\n", file_content);
            kprintf("=== END OF FILE CONTENT ===\n");
        } else {
            kprintf("ERROR: Failed to read file content\n");
        }
        
        kfree(file_content);
    }
}