#include <lib/stdio.h>
#include <i386/ioports.h>
#include <driver/disk/ata.h>

/*
 * Cette fonction permet d'attendre les 400ns d�finit par la norme ATA entre chaque
 * op�ration I/o sur les ports d'un contr�leur
 */
void ata_wait(void)
{
	for (int i = 0; i < 5; i++)
		(void)inb(IDE0_PORT_BASE + ATA_STATUT);
}

/*
 * Cette fonction v�rifie les registres I/O pour d�terminer si une erreur � eu lieu et
 * retourne FALSE si une erreur s'est produite, TRUE sinon.
 */
bool_t ata_result(struct ide_controleur *ctrl)
{
	if (inb(ctrl->io_base + ATA_STATUT) & ATA_ERROR)
		return FALSE;

	return TRUE;
}

/*
 * Cette fonction permet d'attendre un nombre de nano seconde pour attendre la fin
 * d'une op�ration I/O un peu longue, comme par exemple le r�initialisation de tous
 * les disques pr�sent sur un contr�leur
 *
 * TODO: rendre la fonction plus pr�cise en impl�mentant une routine de calibration
 * avec TSC, par exemple
 */
void ata_nano_sleep(uint32_t nano)
{
	while (nano--)
	{
		nano++;
		nano--;
	}
	
}

bool_t ata_is_busy(struct ide_device *dev)
{
	return ((inb(dev->associed_controleur->io_base + ATA_STATUT) & ATA_DRIVE_BUSY) / ATA_DRIVE_BUSY)? 1 : 0;
}

bool_t ata_is_ready(struct ide_device *dev)
{
	return ((inb(dev->associed_controleur->io_base + ATA_STATUT) & ATA_DRIVE_READY) / ATA_DRIVE_READY)? 1 : 0;	
}

uint8_t ata_get_statut(struct ide_device *dev)
{
	return inb(dev->associed_controleur->io_base + ATA_STATUT);
}
/*
 * Cette fonction permet de r�initialiser un contr�leur et ses disques
 *
 * NOTE: R�initialise seulement la partie logicielle des contr�leurs IDE, n'efface pas
 * tous le contenu des disques ;-)
 */
void ata_soft_reset(struct ide_controleur *ctrl)
{
	outb(ctrl->io_base + ATA_DEVICE_CTRL, 0x04);			// R�initialise tous les lecteurs connect�s au contr�leur
	ata_nano_sleep(10000);									// Attend un peu pour que la r�initialisation ait le temps de se faire

	outb(ctrl->io_base + ATA_DEVICE_CTRL, 0x01);			// Fait accepter les commandes au contr�leur
	ata_nano_sleep(10000);									// Attend un peu
}

void ata_select_disk(struct ide_device *dev, uint8_t flags)
{
	uint8_t dev_position = 0;
	
	if (dev->position == IDE_MASTER_DEVICE)
		dev_position = ATA_MASTER;
	else
		dev_position = ATA_SLAVE;

	outbp(dev->associed_controleur->io_base + ATA_DRIVE, (dev_position | flags | ATA_IBM));	
	ata_wait();
}

void ata_send_cmd(struct ide_device *dev, uint8_t cmd, bool_t select_disk)
{
	if(select_disk)
		ata_select_disk(dev, 0);

	outbp(dev->associed_controleur->io_base + ATA_CMD, cmd);
	ata_wait();
}

hret_t ata_read_sectors(struct ide_device *dev, int starting_sector, int sector_count, char *dst_buf)
{
	struct ide_controleur *ctrl = dev->associed_controleur;

	uint16_t data;

	uint8_t low_lba = (starting_sector & 0xFF);
	uint8_t med_lba = ((starting_sector >> 8) & 0xFF);
	uint8_t med_high_lba = ((starting_sector >> 16) & 0xFF);
	uint8_t high_lba = ((starting_sector >> 24) & 0x0F);

	if (sector_count == 0)
		panic("Bien que le standart ATA supporte un nombre de secteur � lire de 0 (=256), HaoudOS ne le supporte pas");

	ata_select_disk(dev, ATA_USE_LBA | high_lba);

	ata_outb(ctrl->io_base + ATA_ERROR, 0x0);							// R�initialise le registre des erreurs
	ata_outb(ctrl->io_base + ATA_SECTOR_COUNT, sector_count & 0xFF);	// Sp�cifie le nombre de secteur � lire
	ata_outb(ctrl->io_base + ATA_SECTOR_NUMBER, low_lba);				// Envoie les 8er bits du secteur de d�part
	ata_outb(ctrl->io_base + ATA_CYL_LSB, med_lba);						// Envoie les 8 bits suivant du secteur de d�part
	ata_outb(ctrl->io_base + ATA_CYL_MSB, med_high_lba);				// Envoie les 8 autre bits suivant du secteur de d�part
	ata_outb(ctrl->io_base + ATA_CMD, ATA_CMD_READ);					// Envoie la commande de lecture

	// Attend que le contr�leur soit pr�t � tranf�rer les donn�es
	while (!(inb(ctrl->io_base + ATA_STATUT) & ATA_DATA_REQUEST));

	for (int i = 0; i < (256 * sector_count); i++)
	{
		iowait();
		data = inw(ctrl->io_base + ATA_DATA);

		// Recopie de la donn�e dans le buffer
		dst_buf[i * 2]		 =	(uint8_t) (data & 0xFF);
		dst_buf[i * 2 + 1]	 =	(uint8_t)((data >> 8) & 0xFF);
	}

	return RET_OK;
}

hret_t ata_write_sectors(struct ide_device *dev, int starting_sector, int sector_count, char *src_buf)
{
	struct ide_controleur *ctrl = dev->associed_controleur;
	uint16_t *buffer;

	uint8_t low_lba = (starting_sector & 0xFF);
	uint8_t med_lba = ((starting_sector >> 8) & 0xFF);
	uint8_t med_high_lba = ((starting_sector >> 16) & 0xFF);
	uint8_t high_lba = ((starting_sector >> 24) & 0x0F);

	if (sector_count == 0)
		panic("Bien que le standart ATA supporte un nombre de secteur � lire de 0 (=256), HaoudOS ne le supporte pas");

	ata_select_disk(dev, ATA_USE_LBA | high_lba);

	ata_outb(ctrl->io_base + ATA_ERROR, 0x0);							// R�initialise le registre des erreurs
	ata_outb(ctrl->io_base + ATA_SECTOR_COUNT, sector_count & 0xFF);	// Sp�cifie le nombre de secteur � lire
	ata_outb(ctrl->io_base + ATA_SECTOR_NUMBER, low_lba);				// Envoie les 8er bits du secteur de d�part
	ata_outb(ctrl->io_base + ATA_CYL_LSB, med_lba);						// Envoie les 8 bits suivant du secteur de d�part
	ata_outb(ctrl->io_base + ATA_CYL_MSB, med_high_lba);				// Envoie les 8 autre bits suivant du secteur de d�part
	ata_outb(ctrl->io_base + ATA_CMD, ATA_CMD_WRITE);					// Envoie la commande d'�criture

	// Attend que le contr�leur soit pr�t � tranf�rer les donn�es
	while (!(inb(ctrl->io_base + ATA_STATUT) & ATA_DATA_REQUEST));

	buffer = (uint16_t *)src_buf;
	for (int i = 0; i < (256 * sector_count); i++)
	{
		outw(ctrl->io_base + ATA_DATA, buffer[i]);					// Et on le transf�re au contr�leur
		iowait();
	}

	return RET_OK;	
}
