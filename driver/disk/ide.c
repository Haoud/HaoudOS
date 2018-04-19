#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <i386/ioports.h>
#include <core/mm/heap.h>
#include <driver/disk/ide.h>
#include <driver/disk/ata.h>
#include <driver/disk/atapi.h>
#include <core/dev/block/block.h>
#include <driver/disk/partition.h>

struct ide_controleur ide_ctrl[IDE_MAX_CONTROLLER];
struct blockdev_ops hdd_ops;

void IdeRegisterDisk(void)
{
	dev_t dev;
	int hdd_num = 0;
	hret_t ret = RET_OK;

	for (int i = 0; i < IDE_MAX_CONTROLLER; i++)
	{
		for(int j = 0; j < IDE_DEVICE_PER_CONTROLLER; j++)
		{
			if(ide_ctrl[i].device[j].type == IDE_DEVICE_NONE)
				continue;

			if(ide_ctrl[i].device[j].type == IDE_DEVICE_NONE)
				continue;

			if(ide_ctrl[i].device[j].type == IDE_DEVICE_HARD_DISK)
			{
				dev = MAKE_DEV(HDD_MAJOR, IDE_MINOR(i, j));
				debugk("[IDE] Registering hdd%1i with dev_t: %08x ...\n", hdd_num, dev);				
				register_blockdev(dev, IDE_BLOCK_SIZE, ide_ctrl[i].device[j].info->lba_capacity, &hdd_ops, &ide_ctrl[i].device[j]);
				
				ret = detect_partitions(dev);
				if(ret != RET_OK)
					debugk("[IDE] Failed to detect partitions on hdd%1i (%08x)\n", hdd_num, dev);				

				hdd_num++;
			}
		}
	}
}

hret_t get_disk_info(struct ide_device *dev)
{
	uint16_t *buffer = kmalloc(512);
	hret_t ret = -ERR_NO_DEV;
	
	if(dev->type != IDE_DEVICE_HARD_DISK)
		goto free_buffer;
	
	ata_send_cmd(dev, ATA_CMD_IDENTIFY, TRUE);	
	
	// TODO: int�grer un timeout
	while(ata_is_busy(dev))
		asm("nop");
		
	ret = -ERR_UNKNOW;
	if(!(ata_get_statut(dev) & ATA_DATA_REQUEST))
		goto free_buffer;

	for(int i = 0; i < 256; i++)
		buffer[i] = inw(dev->associed_controleur->io_base + ATA_DATA);

	memcpy(dev->info, buffer, sizeof(struct ata_device_info));
	kfree(buffer);	
	return RET_OK;

free_buffer:
	kfree(buffer);
	return ret;
}

void Setup_IDE_Disk(void)
{
	debugk("\n");

	// On initialise les structures des contr�leurs
	ide_ctrl[0].id = 0;
	ide_ctrl[1].id = 1;

	ide_ctrl[0].io_base = IDE0_PORT_BASE;
	ide_ctrl[1].io_base = IDE1_PORT_BASE;

	ide_ctrl[0].irq = IDE_CONTROLLER0_IRQ_NR;
	ide_ctrl[1].irq = IDE_CONTROLLER1_IRQ_NR;

	// Par d�faut on consi�dre que les contr�leur sont absent
	ide_ctrl[0].state = IDE_CONTROLEUR_MISSING;
	ide_ctrl[1].state = IDE_CONTROLEUR_MISSING;

	for (int i = 0; i < IDE_MAX_CONTROLLER; i++)
		ConfigureController(&ide_ctrl[i]);

	IdeRegisterDisk();
}

void ConfigureController(struct ide_controleur *ctrl)
{
	/* 
	 * Avant tout on v�rifie que le bus n'est pas "flottant" (floating bus),
	 * c'est � dire qu'il n'y a pas de contr�leur connect� au bus
	 */
	if (inb(ctrl->io_base + ATA_STATUT) == 0xFF)
	{
		debugk("[ATA]: Floating bus detected: controller ID %u (0x%x) not present\n", ctrl->id, ctrl->io_base);
		ctrl->state = IDE_CONTROLEUR_MISSING;
		return;
	}

	// Ici, le contr�leur est pr�sent
	ctrl->state = IDE_CONTROLEUR_PRESENT;

	// Le 1er disque est le disque ma�tre
	ctrl->device[0].support_lba = TRUE;
	ctrl->device[0].position = IDE_MASTER_DEVICE;
	ctrl->device[0].associed_controleur = ctrl;
	ctrl->device[0].type = DetectDisk(&ctrl->device[0]);
	ctrl->device[0].info = kmalloc(sizeof(struct ata_device_info));

	if(ctrl->device[0].type == IDE_DEVICE_HARD_DISK)
		get_disk_info(&ctrl->device[0]);
	
	// Le 2nd disque est le disque esclave
	ctrl->device[1].support_lba = TRUE;
	ctrl->device[1].position = IDE_SLAVE_DEVICE;
	ctrl->device[1].associed_controleur = ctrl;
	ctrl->device[1].type = DetectDisk(&ctrl->device[1]);
	ctrl->device[1].info = kmalloc(sizeof(struct ata_device_info));

	if(ctrl->device[1].type == IDE_DEVICE_HARD_DISK)
		get_disk_info(&ctrl->device[1]);
}

/*
 * Cette fonction permet de d�t�cter si un disque ma�tre/esclave est connect� au
 * contr�leur et d�termine si ce disque (s'il est pr�sent) est un disque dur ou
 * un CD-ROM
 */
enum ide_type DetectDisk(struct ide_device *dev)
{
	uint32_t result;

	uint16_t low_cyl, high_cyl;
	uint16_t io_base = dev->associed_controleur->io_base;

	// R�initialise le contr�leur
	ata_soft_reset(dev->associed_controleur);

	// On sp�cifie le disque que nous souhaitons d�t�cter
	ata_select_disk(dev, 0);

	low_cyl = inb(io_base + ATA_CYL_LSB);
	high_cyl = inb(io_base + ATA_CYL_MSB);
	result = inb(io_base + ATA_STATUT);

	// Si le disque indique �tre occup� c'est qu'aucun disque n'est connect�
	if (result & ATA_DRIVE_BUSY)
		return IDE_DEVICE_NONE;

	if (!(result & (ATA_SEEK_OK | ATA_DRIVE_READY)))
		return IDE_DEVICE_NONE;

	if (low_cyl == 0 && high_cyl == 0)
	{
		debugk("[ATA]: Hard disk found on controller ID %u (0x%x)\n", dev->associed_controleur->id, dev->associed_controleur->io_base);
		return IDE_DEVICE_HARD_DISK;
	}
	else if (low_cyl == ATAPI_MAGIC_LSB && high_cyl == ATAPI_MAGIC_MSB)
	{
		debugk("[ATA/ATAPI]: Optical disk found on controller ID %u (0x%x)\n", dev->associed_controleur->id, dev->associed_controleur->io_base);
		return IDE_DEVICE_CDROM;
	}

	return IDE_DEVICE_NONE;
}

hret_t ide_read(void *custom_data, void *dst_buf, off_t offset)
{
	struct ide_device *dev = custom_data;
	return ata_read_sectors(dev, offset, 1, dst_buf);
}

hret_t ide_write(void *custom_data, void *src_buf, off_t offset)
{
	struct ide_device *dev = custom_data;
	return ata_write_sectors(dev, offset, 1, src_buf);
}

struct blockdev_ops hdd_ops = {
	ide_read,
	ide_write,
	NULL				// Ne sert � rien pour l'instant
};