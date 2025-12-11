#include <ata.h>
#include <fat16.h>
#include <helpers.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <vfs.h>
#include <vfs_conf.h>
#include <block_dev.h>
#include <kmalloc.h>

/* ===== CONSTANTS ===== */

/* FAT16 Boot Sector Signatures */

#define FAT16_BOOT_SIGNATURE	0x29
/* Sizes */
#define FAT16_FAT_ENTRY_SIZE 2
#define FAT16_DIR_ENTRY_SIZE 32
#define FILE_NAME_SIZE 11

/* Special directory entry values */
#define FAT16_DIR_ENTRY_DELETED 0xE5
#define FAT16_DIR_ENTRY_END 0x00

/* Cluster values */
#define FAT16_FREE_CLUSTER		0x0000
#define FAT16_RESERVED_CLUSTER	0x0001
#define FAT16_BAD_CLUSTER		0xFFF7
#define FAT16_EOC				0xFFF8

#define DIV_ROUND_UP(x, y) (((x) + (y)-1) / (y))

static const char name[] = "fat16";
static VfsConf vfs_conf = {0};

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

static u32 cluster_to_lba(Fat16 *fs, u16 cluster) 
{
	return	((cluster - 2) * fs->header.sectors_per_cluster)
		   + fs->data_region_lba;
		   
}

static int read_fat_entry(Fat16 *fs, u16 cluster, u16 *next) 
{
	u32 fat_offset = cluster * FAT16_FAT_ENTRY_SIZE;
	u32 fat_sector = fs->fat_start_lba + (fat_offset / fs->header.bytes_per_sector);
	u32 entry_offset = fat_offset % fs->header.bytes_per_sector;

	char sector[512];
	if (block_dev_read(fs->dev, fat_sector, sector, 1) != 0)
		return -1;

	memcpy(next, sector + entry_offset, sizeof(u16));
	return 0;
}

static u16 get_next_cluster(Fat16 *fs, u16 cluster) 
{
	u16 next = FAT16_EOC;
	read_fat_entry(fs, cluster, &next);
	return next;
}

static void convert_83_to_string(const u8 name[8], const u8 ext[3], char *out_string)
{
	int pos = 0;
	
	for (int i = 0; i < 8; i++)
	{
		if (name[i] != ' ')
			out_string[pos++] = name[i];
	}
	
	if (ext[0] != ' ')
	{
		out_string[pos++] = '.';
		
		for (int i = 0; i < 3; i++)
		{
			if (ext[i] != ' ')
				out_string[pos++] = ext[i];
		}
	}
	
	out_string[pos] = '\0';
}

static int compare_filenames(const char *filename, const Fat16Directory *entry)
{
	char filename_copy[strlen(filename) + 1];
	strcpy(filename_copy, filename);

	for(size_t i = 0; i < strlen(filename_copy); i++)
	   filename_copy[i] = toupper(filename_copy[i]);

	char entry_name[13];
	
	convert_83_to_string(entry->name, entry->extension, entry_name);

	return (strcmp(filename_copy, entry_name));
}

int fat16_detect(BlockDevice *dev)
{
	if (!dev || !dev->present)
		return -1;

	char boot_sector[512];
	FatHeader fs = {0};

	memset(&boot_sector, 0, 512);
	
	if (dev->ops->read(dev, 0, boot_sector, 1) != 0)
		return -1; /* Read error */

	memcpy(&fs, &boot_sector, sizeof(Fat16));

	if (fs.boot_signature == FAT16_BOOT_SIGNATURE)
		return 1;

	return -1;
}

static inline int find_in_root(Fat16 *fs, const char *filename, Fat16Directory *out)
{
	char buffer[512];

	u32 root_dir_entries = fs->header.root_entry_count;
	u32 root_dir_size = root_dir_entries * FAT16_DIR_ENTRY_SIZE;
	u32 root_dir_sectors = DIV_ROUND_UP(root_dir_size, fs->header.bytes_per_sector);
	

	for (u32 sector = 0; sector < root_dir_sectors; sector++) 
	{
		if (block_dev_read(fs->dev, fs->root_dir_lba + sector, buffer, 1) != 0)
			return -1;

		for (u16 i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++) 
		{
			Fat16Directory entry;
			memcpy(&entry, buffer + (i * FAT16_DIR_ENTRY_SIZE), sizeof(Fat16Directory));
			
			if (! is_valid_entry(&entry))
				continue;

			if (compare_filenames(filename, &entry) == 0)
			{
				memcpy(out, &entry, sizeof(Fat16Directory));
				return 0; /* Found */
			}
		}
	}

	return -1; /* Not found */
}

static inline int find_in_dir(Fat16 *fs, u16 start_cluster, const char *filename, Fat16Directory *out)
{
	u16 current_cluster = start_cluster;
	char buffer[512];
	size_t total_entries_checked = 0;

	do {
		u32 lba = cluster_to_lba(fs, current_cluster);

		for (u8 sector = 0; sector < fs->header.sectors_per_cluster; sector++) 
		{
			if (block_dev_read(fs->dev, lba + sector, buffer, 1) != 0)
				return -1;

			for (u16 i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++) 
			{
				Fat16Directory entry;
				memcpy(&entry, buffer + (i * FAT16_DIR_ENTRY_SIZE), sizeof(Fat16Directory));
				
				if(!is_valid_entry(&entry))
					continue;

				total_entries_checked++;

				if (compare_filenames(filename, &entry) == 0) {
					memcpy(out, &entry, sizeof(Fat16Directory));
					return 0; /* Found */
				}
			}
		}
		
		current_cluster = get_next_cluster(fs, current_cluster);
	} while (current_cluster < FAT16_EOC);

	return -1; /* Not found */
}

int fat16_mount(BlockDevice *dev, Fat16 *fs)
{
	if (!dev || !fs)
		return -1;

	fs->dev = dev;
	char buffer[512] = {0};
	if (dev->ops->read(dev, 0, buffer, 1) != 0) {
		kprintf("Error reading boot sector\n");
		return -1;
	}
	memcpy(&fs->header, buffer, sizeof(FatHeader));
	
	/* Calculate LBA values */
	fs->fat_start_lba = fs->header.reserved_sector_count;
	fs->root_dir_lba = fs->fat_start_lba + (fs->header.num_fats * fs->header.fat_size_16);
	fs->data_region_lba = fs->root_dir_lba + 
						 (fs->header.root_entry_count * FAT16_DIR_ENTRY_SIZE + fs->header.bytes_per_sector - 1) / fs->header.bytes_per_sector;
	fs->root_dir_sectors = (fs->header.root_entry_count * FAT16_DIR_ENTRY_SIZE + fs->header.bytes_per_sector - 1) / fs->header.bytes_per_sector;
	return 0;
}

int fat16_read_file(Fat16* fs, Fat16Directory* entry, char* buffer)
{
	if (!fs || !entry || !buffer) {
		return -1;
	}

	u32 file_size = entry->file_size_bytes;
	u16 current_cluster = entry->first_cluster_low;
	u32 bytes_read = 0;
	
	u8 sectors_per_cluster = fs->header.sectors_per_cluster;
	
	u32 data_start_lba = fs->root_dir_lba + 
							 (fs->header.root_entry_count * 32 + 511) / 512;

	char sector_buffer[512];

	
	while (current_cluster >= 0x0002 && current_cluster <= 0xFFEF && bytes_read < file_size)
	{
		u32 cluster_lba = data_start_lba + 
							  (current_cluster - 2) * sectors_per_cluster;
		
		for (int i = 0; i < sectors_per_cluster && bytes_read < file_size; i++) 
		{
			if (block_dev_read(fs->dev, cluster_lba + i, sector_buffer, 1) != 0) {
				kprintf("Error reading sector %u\n", cluster_lba + i);
				return -1;
			}
			
			u32 bytes_to_copy = 512;

			if (bytes_read + bytes_to_copy > file_size) 
				bytes_to_copy = file_size - bytes_read;
			
			memcpy(buffer + bytes_read, sector_buffer, bytes_to_copy);
			bytes_read += bytes_to_copy;
		}
		
		current_cluster = get_next_cluster(fs, current_cluster);
		
		if (current_cluster == 0xFFFF) {
			break;
		}
	}
	
	return 0;
}

int list_directory(Fat16 *fs, const char *path, Fat16Directory *out_entries, size_t max_entries)
{
	Fat16Directory dir = {0};

	find_in_dir(fs, 0, path, &dir);

	size_t entries = 0;

	/* Is in Root*/
	if (strcmp(path, "/") == 0)
	{
		char buffer[512];

		u32 root_dir_entries = fs->header.root_entry_count;
		u32 root_dir_size = root_dir_entries * FAT16_DIR_ENTRY_SIZE;
		u32 root_dir_sectors = DIV_ROUND_UP(root_dir_size, fs->header.bytes_per_sector);
		

		for (u32 sector = 0; sector < root_dir_sectors; sector++) 
		{
			if (block_dev_read(fs->dev, fs->root_dir_lba + sector, buffer, 1) != 0)
				return -1;

			for (u16 i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++) 
			{
				Fat16Directory entry;
				memcpy(&entry, buffer + (i * FAT16_DIR_ENTRY_SIZE), sizeof(Fat16Directory));
				
				if (! is_valid_entry(&entry))
					continue;
				
				/* Yeah that one is good */
				if (entries < max_entries)
				{
					memcpy(&out_entries[entries], &entry, sizeof(Fat16Directory));
					entries++;
				}
				else
				{
					return entries; /* Max entries reached */
				}
			}
		}
	}
	/* Not in root */
	else
	{
		if (!(dir.attributes & FAT16_ATTR_DIRECTORY))
			return -1; /* Not a directory */

		u16 current_cluster = dir.first_cluster_low;
		char buffer[512];

		do {
			u32 lba = cluster_to_lba(fs, current_cluster);

			for (u8 sector = 0; sector < fs->header.sectors_per_cluster; sector++) 
			{
				if (block_dev_read(fs->dev, lba + sector, buffer, 1) != 0)
					return -1;

				for (u16 i = 0; i < fs->header.bytes_per_sector / FAT16_DIR_ENTRY_SIZE; i++) 
				{
					Fat16Directory entry;
					memcpy(&entry, buffer + (i * FAT16_DIR_ENTRY_SIZE), sizeof(Fat16Directory));
					
					if(!is_valid_entry(&entry))
						continue;

					/* Yeah that one is good */

					if (entries < max_entries)
					{
						memcpy(&out_entries[entries], &entry, sizeof(Fat16Directory));
						entries++;
					}
					else
					{
						return entries; /* Max entries reached */
					}
				}
			}
			
			current_cluster = get_next_cluster(fs, current_cluster);
		} while (current_cluster < FAT16_EOC);
	}

	return -1; /* Not found */
}

char** tokenize_path(const char *path, int *token_count)
{
	/* Count slashes in path */
	int count_slash = 0;
	for (const char* p = path; *p; p++) {
		if (*p == '/') count_slash++;
	}

	int N = count_slash + 1;
	
	/* Allocate token matrix: N tokens x 64 characters each */
	char **tokens = (char **)kmalloc(N * sizeof(char *));
	if (!tokens)
	{
		kprintf("Error: Failed to allocate token array\n");
		return NULL;
	}
	
	for (int i = 0; i < N; i++)
	{
		tokens[i] = (char *)kmalloc(64 * sizeof(char));
		if (!tokens[i]) {
			kprintf("Error: Failed to allocate token %u\n", i);
			/* Cleanup previously allocated tokens */
			for (int j = 0; j < i; j++) {
				kfree(tokens[j]);
			}
			kfree(tokens);
			return NULL;
		}
		tokens[i][0] = '\0'; /* Initialize as empty string */
	}

	/* Tokenize path */
	const char *p = path;
	int token_index = 0;
	int char_index = 0;

	/* Skip leading slashes */
	while (*p == '/')
		p++;

	while (*p && token_index < N)
	{
		char_index = 0;

		/* Extract token until next slash or end of string */
		while (*p && *p != '/' && char_index < 63)
		{
			tokens[token_index][char_index] = *p;
			char_index++;
			p++;
		}

		/* Terminate token string */
		tokens[token_index][char_index] = '\0';
		token_index++;

		/* Skip consecutive slashes */
		while (*p == '/')
			p++;
	}

	*token_count = token_index;
	return tokens;
}

int fat16_open(Fat16 *fs, const char *path, Fat16Directory *out)
{
	Fat16Directory current_entry;
	current_entry.first_cluster_low = 0; /* root dir */

	int token_count;
	char **tokens = tokenize_path(path, &token_count);
	if (! tokens)
		return -1;

	/* Search for each token in filesystem */
	int is_root = 1;

	for (int i = 0; i < token_count; i++)
	{
		/* Skip empty tokens */
		if (tokens[i][0] == '\0')
			continue;

		Fat16Directory entry;
		int result;

		if (is_root) 
		{
			result = find_in_root(fs, tokens[i], &entry);
			is_root = 0;
		}
		else
		{
			result = find_in_dir(fs, current_entry.first_cluster_low, tokens[i], &entry);
		}

		if (result != 0)
		{
			/* Cleanup tokens before returning */
			for (int j = 0; j < token_count; j++)
				kfree(tokens[j]);
		
			kfree(tokens);
			return -1;
		}

		memcpy(&current_entry, &entry, sizeof(Fat16Directory));
	}

	/* Cleanup tokens */
	for (int i = 0; i < token_count; i++)
		kfree(tokens[i]);

	kfree(tokens);

	memcpy(out, &current_entry, sizeof(Fat16Directory));
	return 0;
}

void test_fat16()
{
	BlockDevice *dev = block_devs[0];
	
	if (!dev || !dev->present)
	{
		kprintf("No Block device found\n");
		return;
	}

	/* Read FAT16 Reader */
	Fat16 fs = {0};

	fat16_mount(dev, &fs);

	kprintf("FAT16 Info: Root LBA=%u, FAT LBA=%u, DATA LBA=%u\n", fs.root_dir_lba, fs.fat_start_lba, fs.data_region_lba);

	Fat16Directory entry;

	char file_name[64] = "/FOLDER/FILE.TXT";
	
	/* Test simple file first */
	kprintf("\n=== Testing %s ===\n", file_name);
	if(fat16_open(&fs, file_name, &entry) != 0)
	{
		kprintf("File %s not found\n", file_name);
	}
	else
	{
		kprintf("\n\n==========================");
		kprintf("\nSUCCESS: FILE NAME: %s, SIZE: %u bytes, CLUSTER: %u\n", 
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

	Fat16Directory dir_entries[32];
	list_directory(&fs, "/", dir_entries, 32);

	kprintf("\n=== ROOT DIRECTORY ENTRIES:\n");
	for (size_t i = 0; i < 32; i++) 
	{
		if (dir_entries[i].name[0] == 0)
			break; /* No more entries */

		char entry_name[13];
		convert_83_to_string(dir_entries[i].name, dir_entries[i].extension, entry_name);

		kprintf("Entry: %s, Size: %u bytes, Attr: 0x%x\n", 
				entry_name, dir_entries[i].file_size_bytes, dir_entries[i].attributes);
	}
	
	kprintf("\n=== END OF ROOT DIRECTORY ENTRIES ===\n");
}

/* Vfs Interface*/

static int vfs_mount(Vfs *vfs, const char *path, uintptr_t fs_data)
{
	(void)fs_data;
	(void)path;

	vfs->next = NULL;
	vfs->vnode_covered = NULL;
	vfs->flags = 0x00;
	vfs->block_size = 512;
	vfs->data = (uintptr_t)kcalloc(sizeof(Fat16));

	return 0;
}


void fat16_init()
{
	vfs_conf.name = (char*)&name;
	vfs_register(&vfs_conf); /* Config typenum, next */
	vfs_conf.flags = 0x00;

	/* Operations */
	vfs_conf.ops.vfs_mount = &vfs_mount;
}