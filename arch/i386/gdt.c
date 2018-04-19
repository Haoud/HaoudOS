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
#include <assert.h>
#include <i386/gdt.h>
#include <i386/tss.h>
#include <i386/ioports.h>

static struct GdtDescriptor Gdt[GDT_SIZE];		// La GDT en elle même (descripteur de segments)
static struct GdtRegister Gdtr;					// Le registre GDTR pointant sur la GDT

/**
 * @brief Cette fonction initialise la GDT (Glabal Descriptor Table) qui permet d'initialiser la
 * segmentation (obligatoire pour l'architecture x86). Elle procède à quelques vérification
 * sur la taille des descritpeurs de GDT et de la taille de la structure du registre GDTR
 * puis créer les nouveau descripteurs de segments.
 * 
 */
void GdtSetup(void)
{
	BochsPrintf("\n[DEBUG] Address of GDT: 0x%p\n", &Gdt);
	BochsPrintf("[DEBUG] Location of GDT Register: 0x%p\n", &Gdtr);

	SetGdtDescriptor(0, 0x00000000, 0x00000, 0x00, 0x00);		// Segment NULL obligatoire
	SetGdtDescriptor(1, 0x00000000, 0xFFFFF, 0x9A, 0x0C);		// Segment de code du noyau
	SetGdtDescriptor(2, 0x00000000, 0xFFFFF, 0x92, 0x0C);		// Segment de données du noyau

	// Index 3 du la GDT réservé pour le TSS par défaut du noyau

	SetGdtDescriptor(4, 0x00000000, 0xFFFFF, 0xFA, 0x0C);		// Segment de code utilisateur
	SetGdtDescriptor(5, 0x00000000, 0xFFFFF, 0xF2, 0x0C);		// Segment de données utilisateur

	//Initialise la structure GDTR
	Gdtr.base = (uint32_t) Gdt;
	Gdtr.size = GDT_SIZE * GDT_DESC_SIZE;

	FlushGdtr();
	ResetSelectors();
}

/**
 * @brief (Ré)initialise le registre GDTR
 * 
 */
void FlushGdtr(void)
{
	asm("lgdt (Gdtr)");
}

/**
 * @brief (Ré)initialise les sélécteurs de segments avec leurs valeurs par défaut
 * 
 */
void ResetSelectors(void)
{
	asm("mov ax, 0x10		\n\
		 mov ds, ax			\n\
		 mov es, ax			\n\
		 mov fs, ax			\n\
		 mov gs, ax			\n\
		 jmp 0x08:1f		\n\
	 	1:" ::: "eax");
}

/*
 * 
 */

/**
 * @brief Cette fonction permet d'initialiser un descripteur de segment avec les paramètres
 * passés en argument
 * 
 * @param index Numéro du descripteur de segment compris entre 0 et GDT_SIZE
 * @param base L'adresse de base du segment
 * @param limit La limite du segment
 * @param acces Droit d'accès du segment
 * @param flags Options disponibles modifant certains comportements (ex: Taille des données, granularité...)
 */
void SetGdtDescriptor(const int index, const uint32_t base, const uint32_t limit, const uint8_t acces, const uint8_t flags)
{
	if (index >= GDT_SIZE)
		return;

	struct GdtDescriptor *selectedDescriptor = &Gdt[index];

	selectedDescriptor->base0_15 = (uint16_t) (base & 0xFFFF);
	selectedDescriptor->base16_23 = (uint8_t) ((base >> 16) & 0xFF);
	selectedDescriptor->base24_31 = (uint8_t) ((base >> 24) & 0xFF);

	selectedDescriptor->limit0_15 = (uint16_t) (limit & 0xFFFF);
	selectedDescriptor->limit16_19 = (uint8_t) ((limit >> 16) & 0x0F);

	selectedDescriptor->acces = (uint8_t) (acces);
	selectedDescriptor->flags = (uint8_t) (flags & 0x0F);
}