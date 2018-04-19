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

#define HAOUDOS_MAGIC_BOOT		0xE8208D05
#define HAOUDOS_BOOT_SIGNATURE	('B' + ('0' << 8) + ('0' << 16) + ('T' << 24))
#define CD_BOOT			0
#define FLOPPY_BOOT		1
#define DISK_BOOT		2
#define NET_BOOT		3	

struct HaoudOSBootInfo
{
	uint32_t signature;
	uint32_t flags;

	uint32_t kernel_image_size;		// Taille du noyau (en octet) SUR LE DISQUE
	uint32_t lower_mem;				// Quantité de mémoire disponible en mode réel disponible (ie < 1MO)
	uint32_t upper_mem;				// Quantité de mémoire disponible en mode protégé 32 bits (ie < 4GO)
	uint32_t boot_type;				// Est-ce un boot depuis un CD ? Disque dur ? Disquette ?
	uint32_t boot_part;				// Numéro de partition où il faut booter (par défaut 0)
}_packed;
