#pragma once

#define ATAPI_MAGIC_LSB                 0x14
#define ATAPI_MAGIC_MSB                 0xEB

#define ATAPI_DEFAULT_SECTOR_SIZE		2048

// Commande ATAPI
#define ATAPI_CMD_READ					0xA8

void atapi_read_sectors(struct ide_device *dev, int starting_sector, int sector_count, char *dst_buf);
