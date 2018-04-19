#include <assert.h>
#include <lib/stdio.h>
#include <lib/string.h>
#include <lib/stdlib.h>
#include <core/mm/heap.h>
#include <lib/vsnprintf.h>
#include <driver/disk/ide.h>
#include <core/dev/block/block.h>
#include <driver/disk/partition.h>

static bool_t boot_mbr_readed = FALSE;
struct mbr boot_mbr;

/*
 * Cetrte fonction permet d'enregistrer les partitions (primaires seulement) d'un 
 * disque identifié par son ID de périphérique
 */
hret_t detect_partitions(dev_t id)
{
	struct blockdev *blk = NULL;
	struct mbr *_mbr = NULL;
	size_t block_size = 0;						// Taille d'un block
	hret_t ret = RET_OK;

	blk = lookup_blockdev(id);				

	if(blk == NULL)
		return -ERR_NOT_FOUND;

	ret = blockdev_ref(blk);					// Référence le disque dur

	if(ret != RET_OK)
		return ret;	
	
	_mbr = kmalloc(MBR_SIZE);					// Alloue 512 octets pour le mbr

	if(_mbr == NULL)
		return -ERR_NO_MEM;

	/* Lit le MBR */
	block_size = blk->block_size;
	ret = blockdev_kernel_read(blk, 0, _mbr, &block_size);

	if(!boot_mbr_readed)
	{
		memcpy(&boot_mbr, _mbr, MBR_SIZE);
		boot_mbr_readed = TRUE;
	}

	if(ret != RET_OK)
		goto error;

	if(block_size != blk->block_size)
		goto io_error;

	if(_mbr->magic != MBR_MAGIC)
		goto mbr_corrupted;

	// Parcours les 4 entrées de partitions
	for(int i = 0; i < MAX_PRIMARY_PARTITON; i++)
	{
		if(_mbr->parts[i].size == 0)		// Si la partition n'existe pas
			continue;						// Bah on passe à la suivante

		ret = register_partition(id + i + 1, blk, _mbr->parts[i].size, _mbr->parts[i].lba, NULL);

		if(ret != RET_OK)
			debugk("[PARTITION] Failed to register partition\n");
		else
			debugk("[PARTITION] Partition id %08x registered\n", id + i + 1, blk);
	}

	kfree(_mbr);				// Libère la mémoire
	blockdev_unref(blk);		
	return RET_OK;

mbr_corrupted:
	debugk("[PARTITION] MBR corrupted: Unable to detect partitions\n");
	goto error;
io_error:
	debugk("[PARTITION] MBR reading failure: Unable to detect partitions\n");
	ret = ERR_IO;
error:
	if(_mbr)
		kfree(_mbr);

	if(blk)
		blockdev_unref(blk);

	return ret;
}

/*
 * Cette fonction permet de désenregistrer toutes les partitions d'un disque
 * identifié par son ID de périphérique
 */ 
hret_t unregister_partitions(dev_t _unused id)
{
	return -ERR_NOT_IMPL;
}

hret_t write_parts_on_dev(char *path)
{
	char device[IDE_MAX_DEVICE * IDE_MAX_CONTROLLER] = {'a', 'b', 'c', 'd'};
	size_t size_to_alloc = strlen(path) + 10;
	char *devpath = kmalloc(size_to_alloc);

	// On créer les fichiers périphériques nécessaires des disques et partitions
	for(int i = 0; i < IDE_MAX_DEVICE * IDE_MAX_CONTROLLER; i++)
	{
		if(blkdev_present(MAKE_DEV(HDD_MAJOR, i * MAX_PART_PER_DISK)))
		{
			snprintf(devpath, size_to_alloc, "%s\\hd%c", path, device[i]);
			make_speacial_node(devpath, &process_list[0], INODE_BLOCK_DEVICE, MAKE_DEV(HDD_MAJOR, 0));

			for(int j = 0; j < MAX_PART_PER_DISK; j++)
			{
				if(blkdev_present(MAKE_DEV(HDD_MAJOR, i + j + 1)))
				{
					snprintf(devpath, size_to_alloc, "%s\\hd%c%u", path, device[i], j + 1);
					make_speacial_node(devpath, &process_list[0], INODE_BLOCK_DEVICE, MAKE_DEV(HDD_MAJOR, i * MAX_PART_PER_DISK + j + i));
				}
			}
		}
	}

	kfree(devpath);
	return RET_OK;
}