/*
 * This file was created on Tue Mar 27 2018
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
#include <i386/tss.h>
#include <i386/gdt.h>
#include <lib/stdlib.h>
#include <driver/bochs/bochs.h>

struct Tss tss_kernel;								// Un seul TSS pour tout le système

/**
 * @brief Cette fonction initialise tout ce qui concerne le TSS et affiche quelques messages de
 * débogage sur le 0xe9 de bochs
 * 
 */
void SetupTss(void)
{
	memset((void *)&tss_kernel, 0, sizeof(struct Tss));									// Mise à zéro de la structure tss_kernel

	InstallTss();	
	FlushTss();

	BochsPrint("[INFO] TSS are ready, dumping structure...\n");
	BochsDump((void *)&tss_kernel, sizeof(struct Tss));
}

/**
 * @brief Cette fonction installe dans la GDT le descripteur de TSS à l'index n°3
 * qui lui est réservé: l'offset est donc 0x18 (8 * 3)
 */
void InstallTss(void)
{
	BochsPrintf("[INFO]: Updating TSS...\n");
	SetGdtDescriptor(3, (uint32_t)&tss_kernel, sizeof(tss_kernel), 0xE9, 0x00);
	BochsPrintf("[INFO]: TSS updated !\n");
}
/**
 * @brief Cette fonction permet de réinitialiser le registre TR qui contient l'offset
 * dans le GDT du descripteur de TSS.
 */
void FlushTss(void)
{
	asm("pushf			\n\
		 cli			\n\
		 mov ax, 0x18	\n\
		 ltr ax			\n\
		 popf" ::: "eax");
}