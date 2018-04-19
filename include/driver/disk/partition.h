#pragma once
#include <types.h>
#include <core/dev/dev.h>


#define MAX_PRIMARY_PARTITON	4			// Nombre maximum de partitions primaires
#define MAX_PART_PER_DISK		32			// Nombre maximum de partitions secondaires support�es par HaoudOS (en comptant le disque dur comme une partition)
#define HAOUDOS_BOOT_TYPE 		0x5a		// Partiton de boot d'HaoudOS
#define MBR_MAGIC				0xAA55		// Nombre magique du MBR
#define MBR_SIZE				512

struct partition
{
	uint8_t bootable;
	uint8_t start_head;			// T�te de d�part
	uint16_t start_cyl;			// Cylindre de d�part
	uint8_t type;				// Identificateur de la partition (
	uint8_t end_head;			// T�te de fin
	uint16_t end_cyl;			// Cylindre de fin
	uint32_t lba;				// LBA du d�part de la partition
	uint32_t size;				// Nombre de secteurs
}_packed;

struct mbr
{
	uint8_t bootcode[446];			// Le code de d�marrage
	struct partition parts[4];		// 4 partitions
	uint16_t magic;					// Normalement 0xAA55
}_packed;

extern struct mbr boot_mbr;

hret_t detect_partitions(dev_t id);
hret_t unregister_partitions(dev_t id);

hret_t write_parts_on_dev(char *path);