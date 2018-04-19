/*
 * This file was created on Wed Mar 28 2018
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
#pragma once
#include <types.h>

#define IDT_SIZE			256					// Taille de l'IDT (nombre d'entrées)
#define IDT_DESC_SIZE		8					// Taille d'une entrée de l'IDT

#define IDT_INT_GATE32			0x0E
#define IDT_INT_GATE16			0x06			

#define IDT_TRAP_GATE32			0x0F
#define IDT_TRAP_GATE16			0x07

#define IDT_TASK_GATE			0x05

#define IDT_HANDLER_PRESENT		0x80			// Indique que le handler de cet interruption est présent
#define IDT_USER_ACCES			0x60			// Autorise l'utilisateur à accéder à cette interruption vonlontairement

struct IdtDescriptor
{
	uint16_t offset0_15;		// Offset bas de l'adresse de la fonction appelée
	uint16_t selector;			// Sélécteur de code utilisé lors d'une interruption
	uint8_t unused;				// Ne sert à rien
	uint8_t acces;				// Droits d'accès
	uint16_t offset15_31;		// Offset haut de l'adresse de la fonction appelée
}_packed;	

struct IdtRegister
{
	uint16_t limit;
	uint32_t base;
}_packed;

void FlushIdtr(void);
void GenerateIDT(void);
void MakeIdtDescriptor(uint32_t index, uint32_t fn, uint16_t selector, uint8_t acces);

