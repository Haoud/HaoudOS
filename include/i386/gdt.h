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

#define GDT_SIZE		6			// Maximum 5 entrée dans la GDT (sans compter l'entrée 0 qui ne peut pas être utilisée)
#define GDT_DESC_SIZE	0x08		// Taille d'un descripteur de segment (en octets)

struct GdtDescriptor
{
	uint16_t limit0_15;
	uint16_t base0_15;
	uint8_t base16_23;
	uint8_t acces;
	uint8_t limit16_19 : 4;
	uint8_t flags : 4;				
	uint8_t base24_31;
}_packed;

struct GdtRegister
{
	uint16_t size;
	uint32_t base;
}_packed;

void GdtSetup(void);
void FlushGdtr(void);
void ResetSelectors(void);
void SetGdtDescriptor(const int index, const uint32_t base, const uint32_t limit, const uint8_t acces, const uint8_t flags);