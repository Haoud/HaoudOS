#pragma once
#include <types.h>

#define IDE_MAX_DEVICE					2
#define IDE_MAX_CONTROLLER				2

#define IDE_CONTROLLER0_IRQ_NR			14
#define IDE_CONTROLLER1_IRQ_NR			15

#define IDE_DEVICE_PER_CONTROLLER		2

#define IDE_BLOCK_SIZE					512										// Taille d'une bloc pour les disques dur IDE

#define IDE_DEVICE(ctrl,device)			(((ctrl) << 1) + (device))
#define IDE_MINOR(ctrl,device)			(IDE_DEVICE(ctrl,device) * 32)			// Calcule le nombre mineur d'un disque dur en fonction de sa position et du numéro de son contrôleur

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
	bool_t support_lba;								// Nécessaire pour HaoudOS
	struct ata_device_info *info;					// Nécessaire pour HaoudOS pour la taille du disque, des secteurs, etc...

	enum ide_type type;								// Type de disque (disque dur, CD-ROM ou rien du tout)
	enum ide_position position;						// Position du disque par rapport au contrôleur (Maître ou esclave)
	struct ide_controleur *associed_controleur;		// Controleur associé au périphérique
};

struct ide_controleur
{
	uint32_t id;									// Numéro du contrôleur en partant de 0
	uint32_t irq;									// IRQ associé au contrôleur
	uint16_t io_base;								// Port de base pour les opération I/O

	enum controleur_state state;							// Etat du contrôleur
	struct ide_device device[IDE_DEVICE_PER_CONTROLLER];	// Périphériques connectés au contrôleur
};

extern struct ide_controleur ide_ctrl[];			// Maximum 2 contrôleurs

void Setup_IDE_Disk(void);
enum ide_type DetectDisk(struct ide_device *dev);
void ConfigureController(struct ide_controleur *ctrl);
