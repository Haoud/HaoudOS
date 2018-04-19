#include <lib/stdio.h>
#include <i386/ioports.h>
#include <driver/disk/ata.h>
#include <driver/disk/ide.h>
#include <driver/disk/atapi.h>

/*
 * Cette fonction permet de lire lire un ou plusieurs secteurs d'un CD-ROM et place les donn�es lues
 * dans le buffer de destination.
 * NOTE: Le p�riph�rique dev doit pointer sur un lecteur CD-ROM
 */
void atapi_read_sectors(struct ide_device *dev, int starting_sector, int sector_count, char *dst_buf)
{
	uint8_t atapi_cmd[12] = { ATAPI_CMD_READ, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };				// Commande SCSI et ses arguments
	struct ide_controleur *ctrl = dev->associed_controleur;	
	uint16_t io_base = ctrl->io_base;
	uint16_t data;
	uint8_t statut;

	ata_select_disk(dev, 0);																	// On s�l�ctionne le disque CD-ROM que l'on veut lire
	ata_outb(io_base + ATA_PRECOMP, 0);															// Pr�compensation ignor� sur la plupart des processeurs
	ata_outb(io_base + ATA_CYL_LSB, ATAPI_DEFAULT_SECTOR_SIZE & 0xFF);							// On sp�cifie la taille des secteurs du CD-ROM
	ata_outb(io_base + ATA_CYL_LSB, (ATAPI_DEFAULT_SECTOR_SIZE >> 8) & 0xFF);					// On sp�cifie la taille des secteurs du CD-ROM
	ata_outb(io_base + ATA_CMD, ATA_CMD_PACKET_COMMAND);										// Demande de pr�paration d'envoie d'une commande SCSI

	// Tant que le disque est occup�
	statut = ATA_DRIVE_BUSY;

	while (statut & ATA_DRIVE_BUSY)
		statut = inb(io_base + ATA_STATUT);

	// Tant que l'on peut pas tranf�rer des donn�es (ou qu'une erreur est survenue)
	statut = 0x0;
	while (!(statut & ATA_DATA_REQUEST) && !(statut & ATA_ERROR))
		statut = inb(io_base + ATA_STATUT);

	// Si erreur on arr�te tout (pour facilit� le d�bogage)
	if (statut & ATA_ERROR)
		panic("Erreur de lecture ATAPI\n");

	// On pr�pare la commande SCSI
	atapi_cmd[9] = (uint8_t) sector_count;								// Nombre de secteurs � lire	
	atapi_cmd[2] = (uint8_t) ((starting_sector >> 24) & 0xFF);	// On envoie par packet de 8 bits le secteur de d�part
	atapi_cmd[3] = (uint8_t) ((starting_sector >> 16) & 0xFF);	// Identique
	atapi_cmd[4] = (uint8_t) ((starting_sector >> 8) & 0xFF);		// Idem
	atapi_cmd[5] = (uint8_t) (starting_sector & 0xFF);			// Bon... vous avez compris le principe je pense :-)

	// On envoie la commande SCSI
	for (int i = 0; i < 6; i++)
		outw(io_base + ATA_DATA, ((uint16_t *)atapi_cmd)[i]);

	// On attend que le commande SCSI a �t� prise en compte
	while (!(inb(io_base + ATA_STATUT) & ATA_DATA_REQUEST));

	// On r�cup�re les donn�es du CD-ROM
	for (int i = 0; i < ((ATAPI_DEFAULT_SECTOR_SIZE / 2) * sector_count); i++)
	{
		data = inw(ctrl->io_base + ATA_DATA);

		// Recopie de la donn�e dans le buffer
		dst_buf[i * 2] = (uint8_t)data;
		dst_buf[i * 2 + 1] = (uint8_t)(data >> 8);
	}
}