/*
 * This file was created on Fri Apr 13 2018
 * Copyright 2018 Romain CADILHAC
 *
 * This file is a part of HaoudOS.
 *
 * HaoudOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HaoudOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with HaoudOS. If not, see <http://www.gnu.org/licenses/>.
 */
#include <boot.h>
#include <types.h>
#include <const.h>
#include <assert.h>
#include <usr/init.h>
#include <i386/fpu.h>
#include <i386/gdt.h>
#include <i386/idt.h>
#include <i386/tss.h>
#include <i386/tsc.h>
#include <lib/stdio.h>
#include <usr/shell.h>
#include <core/shell.h>
#include <i386/i8259.h>
#include <i386/i8254.h>
#include <i386/paging.h>
#include <core/fs/vfs.h>
#include <i386/ioports.h>
#include <core/mm/phys.h>
#include <core/mm/heap.h>
#include <core/mm/area.h>
#include <lib/vsnprintf.h>
#include <core/time/cmos.h>
#include <core/time/time.h>
#include <core/time/timer.h>
#include <core/process/elf.h>
#include <driver/disk/ide.h>
#include <driver/disk/ata.h>
#include <driver/vga/text.h>
#include <i386/exceptions.h>
#include <core/fs/ext2/ext2.h>
#include <core/dev/char/tty.h>
#include <driver/disk/atapi.h>
#include <driver/bochs/bochs.h>
#include <core/mm/mm_context.h>
#include <core/process/sleep.h>
#include <core/dev/block/block.h>
#include <core/syscall/syscall.h>
#include <core/process/process.h>
#include <driver/disk/partition.h>
#include <core/dev/char/urandom.h>
#include <core/process/schedule.h>
#include <core/fs/ramdisk/ramdisk.h>
#include <driver/keyboard/keyboard.h>


extern unsigned int _kernel_end;

#ifdef DEBUG_MODE
	#define FN_INIT_OK()									\
		GoTo(70, -1);										\
		PrintColor("OK\n", LIGHT_GREEN, BLACK);

	#define INIT_PRINT(s)	Print(s);
#else
	#define FN_INIT_OK()
	#define INIT_PRINT(s)	
#endif

void cd_boot(struct HaoudOSBootInfo *BootData);

/*
 * Cette fonction est le premier processus cr�� et ne fait RIEN en mode release
 * En mode DEBUG, ce processus affiche des informations de d�buguage dans une
 * r�serv�e par le noyau de la console
 */
void idle(void)
{
	for(;;)
		asm volatile("hlt");
}

/*
 * bdflush: Buffer Dirty FLUSH
 * 
 * Thread noyau appel� tous les 30 secondes afin de garder le syst�me de fichier synchronis�
 * avec le disque dur
 */
void bdflush(void)
{
	for(;;)
	{
		schedule_timeout(30000);		// On patiente 30 secondes
#ifdef DEBUG_MODE
		debugk("[BDFLUSH]: Syncing dirty inodes...\n");
#endif
		do_sync();
	}
}

/*
 * Cette fonction est appel� par le point d'entr�e du fichier KERNEL32.EXE et
 * se charge d'initialiser HaoudOS en appelant les diff�rantes m�thodes utiles
 * pour l'initialisation du noyau
 *
 * Cette fonction est le coeur d'HaoudOS: si une fonction dans cette m�thode est
 * d�fectueuse, il se peut que tout le noyau soit inutilisable
 */
int startup32(const uint32_t magic, struct HaoudOSBootInfo *BootData)
{
	BochsSetup();
	BochsPrint("Hello from Bochs !\n");
	BochsPrint("Bochs text debugger initialized\n\n");

	AssertFatal(magic == HAOUDOS_MAGIC_BOOT);		// V�rification de l'origine du bootloader et/ou de la validit� de la structure BootData

	BochsPrintf("[DEBUG] BootData are located at 0x%p\n", BootData);
	BochsPrintf("[DEBUG] Signature of BootData are 0x%X\n", BootData->signature);

	AssertFatal(BootData->signature == HAOUDOS_BOOT_SIGNATURE);

	// On doit initialiser le driver VGA avant d'afficher quoi que ce soit � l'�cran 
	VgaTextSetup();
	Print("D�marrage d'HaoudOS ... \n");
	printk("Quantit� de m�moire d�t�ct�e: %u ko lower, %u Mo upper\n", BootData->lower_mem, BootData->upper_mem / 1024);

	printk("[INFO] Kernel image size: %u ko\n", BootData->kernel_image_size / 1024);

	BochsPrintf("\n[INFO] Lower memory avaible: %u o = %u ko\n", BootData->lower_mem * 1024, BootData->lower_mem);
	BochsPrintf("[INFO] Upper memory avaible: %u o = %u ko = %u Mo\n", BootData->upper_mem *1024, BootData->upper_mem, BootData->upper_mem / 1024);

#ifdef RELEASE_MODE
	ClearScreen();
	PrintColor(" ", BLUE, BLUE);
	PrintColor(" ", WHITE, WHITE);
	PrintColor(" ", RED, RED);
	printk(" %s %s\n", OS_NAME, OS_VERSION);
#endif

	INIT_PRINT("Pr�paration du CPU...");
	cpuid_init();
	FN_INIT_OK();

	INIT_PRINT("Chargement de la GDT...");
	GdtSetup();
	FN_INIT_OK();

	INIT_PRINT("Initialisation du TSS...");
	SetupTss();
	FN_INIT_OK();

	INIT_PRINT("Ramappage des IRQs et reprogrammation du PIC i8259...");
	i8259_AutoRemap();
	FN_INIT_OK();

	INIT_PRINT("G�n�ration de l'IDT par d�faut...");
	GenerateIDT();
	FN_INIT_OK();

	INIT_PRINT("Pr�paration des exceptions du processeur");
	SetupExceptions();
	FN_INIT_OK();
	
	INIT_PRINT("Initialisation du FPU...");
	FpuSetup();
	FN_INIT_OK();

	INIT_PRINT("Configuration du PIT i8254...");
	i8254_SetFrenquency(PIT_8254_DEFAULT_FRENQUENCY);				// 100 HZ, soit une interruption toutes les 10 ms
	FN_INIT_OK();

	INIT_PRINT("Pr�paration des timers dynamiques...");
	init_timer_driver();
	FN_INIT_OK();

	INIT_PRINT("Pr�paration de la CMOS...");
	cmos_init();
	FN_INIT_OK();

	INIT_PRINT("Pr�paration de la gestion du temps et de la date...");
	init_time();
	FN_INIT_OK();

	INIT_PRINT("Configuration du clavier PS/2 ...");
	SetupKeyboard();
	FN_INIT_OK();

	INIT_PRINT("Pr�paration de la gestion de la m�moire physique...");
	PhysMemSetup(BootData->upper_mem, (0x100000 + BootData->kernel_image_size));
	FN_INIT_OK();

	INIT_PRINT("Initialisation de la pagination...");
	SetupPaging();
	FN_INIT_OK();

	INIT_PRINT("Pr�paration du tas du noyau...");
	SetupHeap(PHYS_ALIGN_SUP(GetEndOfMemMap()));		// Le heap se situe sur la page apr�s la table des descripteurs de pages physiques 
	FN_INIT_OK();

	INIT_PRINT("Pr�paration de la gestion des zones m�moires contigues...");
	SetupVmArea(PHYS_ALIGN_SUP(GetEndOfMemMap()) + HEAP_INITIAL_SIZE, MIRRORING_ADDR - 1);
	FN_INIT_OK();

	INIT_PRINT("Pr�paration de la gestion des contextes m�moire...");
	Setup_MMContext_Kernel();
	FN_INIT_OK();

	INIT_PRINT("Pr�paration du scheduler...");
	SchedulerSetup();
	FN_INIT_OK();

	INIT_PRINT("Pr�paration des appels syst�mes...");
	SyscallSetup();
	FN_INIT_OK();
	
	INIT_PRINT("Pr�paration des disques...");
	Setup_IDE_Disk();
	FN_INIT_OK();

	INIT_PRINT("Mise en route du VFS...");
	PrepareVFS();										// Pr�paration du VFS
	RegisterRamdisk();									// Le Ramdisk est pris en compte par le VFS
	RegisterExt2();										// Syst�me de fichier d'HaoudOS par d�faut
	SetupVFS("ramdisk", NULL, 'B');						// Mise en route du VFS

	FN_INIT_OK();

	// NE PAS CHANGER L'ORDRE DE CREATION DES PROCESSUS SUIVANTS !!!
	create_kthread(&idle, PAGE_SIZE);					// Doit toujours �tre le 1er processus � �tre cr��
	create_kthread(&bdflush, PAGE_SIZE);				// Permet d'�viter la corruption du disque dur
	create_process(&init, PAGE_SIZE * 2);				// Permet de terminer l'initialisation du syst�me et de lancer le shell

	// Ici, on peut utiliser le FPU sans risques

	INIT_PRINT("Detection du TSC...");
	DetectTSC();
	FN_INIT_OK();

	INIT_PRINT("Initialisation du terminal...");
	tty_init();
	FN_INIT_OK();

	INIT_PRINT("Initialisation du g�n�rateur de nombres pseudo-al�atoires...");
	init_urandom();
	FN_INIT_OK();
	
	BochsPrint("\n\n[HAOUD OS]: System are ready !\n");
	print_mem_stat();

	/* Pr�pare HaoudOS pour l'�x�cution de la t�che init */
	switch(BootData->boot_type)
	{
		case CD_BOOT:
			cd_boot(BootData);
			break;

		case DISK_BOOT:
		case FLOPPY_BOOT:
		case NET_BOOT:
		default:
			panic("Type de boot non support�");
			break;
	}

	execute_idle();
	return 0;
}

void cd_boot(struct HaoudOSBootInfo *BootData)
{
	char path[16];
	write_parts_on_dev("B:");

	snprintf(path, 16, "B:\\hda%u", BootData->boot_part + 1);
	mount('C', "ext2", path);										// D�marre sur la partition d�sign�e par le bootloader
	umount('B');													// D�monte le syst�me de fichier temporaire

	write_parts_on_dev("C:\\dev");	
}