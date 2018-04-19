#pragma once
#include <types.h>

#define IDE_MAX_DEVICE					2
#define IDE_MAX_CONTROLLER				2

#define IDE_CONTROLLER0_IRQ_NR			14
#define IDE_CONTROLLER1_IRQ_NR			15

#define IDE_DEVICE_PER_CONTROLLER		2

#define IDE_BLOCK_SIZE					512										// Taille d'une bloc pour les disques dur IDE

#define IDE_DEVICE(ctrl,device)			(((ctrl) << 1) + (device))
#define IDE_MINOR(ctrl,device)			(IDE_DEVICE(ctrl,device) * 32)			// Calcule le nombre mineur d'un disque dur en fonction de sa position et du num�ro de son contr�leur

enum ide_type
{
	IDE_DEVICE_NONE,
	IDE_DEVICE_HARD_DISK,
	IDE_DEVICE_CDROM
};

enum ide_position
{
	IDE_MASTER_DEVICE,
	IDE_SLAVE_DEVICE
};

enum controleur_state
{
	IDE_CONTROLEUR_MISSING,
	IDE_CONTROLEUR_PRESENT
};

struct ide_device
{
	bool_t support_lba;								// N�cessaire pour HaoudOS
	struct ata_device_info *info;					// N�cessaire pour HaoudOS pour la taille du disque, des secteurs, etc...

	enum ide_type type;								// Type de disque (disque dur, CD-ROM ou rien du tout)
	enum ide_position position;						// Position du disque par rapport au contr�leur (Ma�tre ou esclave)
	struct ide_controleur *associed_controleur;		// Controleur associ� au p�riph�rique
};

struct ide_controleur
{
	uint32_t id;									// Num�ro du contr�leur en partant de 0
	uint32_t irq;									// IRQ associ� au contr�leur
	uint16_t io_base;								// Port de base pour les op�ration I/O

	enum controleur_state state;							// Etat du contr�leur
	struct ide_device device[IDE_DEVICE_PER_CONTROLLER];	// P�riph�riques connect�s au contr�leur
};

extern struct ide_controleur ide_ctrl[];			// Maximum 2 contr�leurs

void Setup_IDE_Disk(void);
enum ide_type DetectDisk(struct ide_device *dev);
void ConfigureController(struct ide_controleur *ctrl);
