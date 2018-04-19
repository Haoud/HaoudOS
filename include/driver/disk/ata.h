#pragma once
#include "ide.h"
#include <types.h>

#define IDE0_PORT_BASE				0x1F0		// Port de "base" pour le disque dur ma�tre
#define IDE1_PORT_BASE				0x170		// Port de "base" pour le disque dur esclave

#define ATA_DATA					0x00		// Offset � rajouter � IDE0_PORT_BASE ou IDE1_PORT_BASE
#define ATA_ERROR					0x01		// Registre d'erreurs
#define ATA_PRECOMP					0x01		// Registre de pr�compensation
#define ATA_SECTOR_COUNT			0x02		// Nombre de secteurs � lire/�crire
#define ATA_SECTOR_NUMBER			0x03		// Secteur de d�part pour la lecture/�criture
#define ATA_CYL_LSB					0x04		// 16 bits de poind faible
#define ATA_CYL_MSB					0x05		// 16 bits de poids fort
#define ATA_DRIVE					0x06		// Pour sp�cifier un disque & d'autres options
#define ATA_STATUT					0x07		// Statut du contr�leur IDE (lecture seule)
#define ATA_CMD						0x07		// Pour envoyer une commande (�criture seule)

#define ATA_STATUT_BIS				0x206		// Pareil que le port ATA_STATUT mais lors de la lecture de ce port l'IRQ associ� au disque est annihil�e
#define ATA_DEVICE_CTRL				0x206		// Permet de contr�ler certains comportements du controleur du disque

// Flags du port ATA_DRIVE
#define ATA_IBM						0xA0		// Ces bits doivent �tre positionn�s (standart d'IBM)
#define ATA_USE_LBA					0x40		// On souhaite utiliser la notation LBA au lieu de la notation CHS
#define ATA_MASTER					0x00		// On souhaite utiliser le disque ma�tre
#define ATA_SLAVE					0x10		// On souhaite utiliser le disque esclave

// Flags du port ATA_STATUT
#define ATA_ERROR					0x01		// Une erreur s'est produite
#define ATA_INDEX					0x02		// Index du disque pass� ?
#define ATA_ERR_CORR				0x04		// Erreur corrig� gr�ce au octets ECC
#define ATA_DATA_REQUEST			0x08		// Des donn�es peuvent t'elle �tre transf�rer ?
#define ATA_SEEK_OK					0x10		// D�placement de la t�te de lecture OK
#define ATA_WRITE_FAULT				0x20		// Erreur d'�criture
#define ATA_DRIVE_READY				0x40		// Le disque est pr�t
#define ATA_DRIVE_BUSY				0x80		// Le disque est occup� (il travaille)

// Flags du port ATA_ERROR
#define ATA_MEDUIM_CHANGE_REQUIRED	0x01		// Changement de disque requis
#define ATA_MEDUIM_CHANGED			0x02		// Disque CD-ROM chang�
#define ATA_NO_DATA_MARK			0x04		// Marqueur d'adresse de donn�es non trouv�
#define ATA_TRACK0_NOT_FOUND		0x08		// Piste 0 non trouv�e
#define ATA_ABORTED					0x10		// Commande avort�e
#define ATA_NOT_ID					0x20		// Marqueur ID non trouv�e
#define ATA_ERROR_UNCORRECTABLE		0x40		// Erreur non corrigeable avec les octets ECC
#define ATA_BAD_BLOCK				0x80		// Secteur marqu� endommag� par l'h�te

// Commande pour le port ATA_CMD
#define ATA_CMD_CALIBRATE			0x10		// Calibrer le lecteur
#define ATA_CMD_READ				0x20		// Lecture de secteurs
#define ATA_CMD_WRITE				0x30		// Ecriture de secteurs
#define ATA_CMD_VERIFY				0x40		// V�rifications de secteurs
#define ATA_CMD_FORMAT				0x50		// Formatage d'une piste
#define ATA_CMD_SEARCH				0x70		// D�place les t�ted du disque sur une piste
#define ATA_CMD_DIAGNOSTIC			0x90		// Diagnostique la partie �l�ctronique du contr�leur
#define ATA_CMD_PACKET_COMMAND		0xA0		// Demande d'envoie d'une commande SCSI (ATAPI)
#define ATA_CMD_IDENTIFY			0xEC		// Identifie les lecteurs connect�s


// Commande pour le port ATA_DEVICE_CTRL
#define ATA_DISK_IRQ_ENABLE			0x02		// Activer l'IRQ disque apr�s chaque commande
#define ATA_SYSTEM_RESET			0x04		// R�initialiser tous les lecteurs connect�s

#define ata_outb(port, data)		\
	outb(port, data);				\
	ata_wait();

// Structure retourn�e lors d'une commande ATA_CMD_IDENTIFY
struct ata_device_info
{
	uint16_t general_config_info;     
	uint16_t nb_logical_cylinders;     
	uint16_t reserved1;                
	uint16_t nb_logical_heads;        
	uint16_t unformatted_bytes_track;  
	uint16_t unformatted_bytes_sector; 
	uint16_t nb_logical_sectors;      
	uint16_t vendor1[3];               
	uint8_t  serial_number[20];       
	uint16_t buffer_type;          
	uint16_t buffer_size;             
	uint16_t ecc_bytes;               
	uint8_t  firmware_revision[8];     
	uint8_t  model_number[40];        
	uint8_t  max_multisect;            
	uint8_t  vendor2;
	uint16_t dword_io;                 
	uint8_t  vendor3;                  
	uint8_t  capabilities;
	uint16_t reserved2;                
	uint8_t  vendor4;                  
	uint8_t  pio_trans_mode;
	uint8_t  vendor5;                  
	uint8_t  dma_trans_mode;
	uint16_t fields_valid;             
	uint16_t cur_logical_cylinders;    
	uint16_t cur_logical_heads;        
	uint16_t cur_logical_sectors;      
	uint16_t capacity1;              
	uint16_t capacity0;               
	uint8_t  multsect;        
	uint8_t  multsect_valid;
	uint32_t lba_capacity;            
	uint16_t dma_1word;                
	uint16_t dma_multiword;         
	uint16_t pio_modes;           
	uint16_t min_mword_dma;           
	uint16_t recommended_mword_dma;   
	uint16_t min_pio_cycle_time;     
	uint16_t min_pio_cycle_time_iordy;
	uint16_t reserved3[11];         
	uint16_t major_version;          
	uint16_t minor_version;       
	uint16_t command_sets1;         
	uint16_t command_sets2;            
	uint16_t reserved4[4];           
	uint16_t dma_ultra;                
	uint16_t reserved5[37];           
	uint16_t last_lun;                
	uint16_t reserved6;                
	uint16_t security;                 
	uint16_t reserved7[127];
}_packed;

void ata_wait(void);
void ata_nano_sleep(uint32_t nano);
bool_t ata_is_busy(struct ide_device *dev);
bool_t ata_is_ready(struct ide_device *dev);
bool_t ata_result(struct ide_controleur *ctrl);
uint8_t ata_get_statut(struct ide_device *dev);
void ata_soft_reset(struct ide_controleur *ctrl);
void ata_select_disk(struct ide_device *dev, uint8_t flags);
void ata_send_cmd(struct ide_device *dev, uint8_t cmd, bool_t select_disk);

hret_t ata_read_sectors(struct ide_device *dev, int starting_sector, int sector_count, char *dst_buf);
hret_t ata_write_sectors(struct ide_device *dev, int starting_sector, int sector_count, char *src_buf);
