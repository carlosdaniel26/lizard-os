#ifndef RAMDISK_H
#define RAMDISK_H

void ramdisk_init(uint32_t sector_ammount);
uint8_t *ramdisk_read_sector(uint32_t sector);
void ramdisk_write_sector(uint8_t *sector_data, uint32_t sector);

#endif