#include <stdint.h>

uint16_t ata_identify(uint8_t drive_id);
int ata_write_sector(uint8_t drive_id, uint32_t lba, const char *buffer);