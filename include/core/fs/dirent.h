/*
 * This file was created on Wed Apr 11 2018
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
#include <core/fs/inode.h>

#define DIRENT_MAX_NAME_SIZE		255

struct dirent
{
	uint64_t inode_id;							// Identificateur de l'inode de l'entrée du répertoire
	uint32_t offset_in_dirfile;					
	enum inode_type type;						// Type de l'inode
	char name[DIRENT_MAX_NAME_SIZE + 1];		// 255 caractères max sans le \0 de fin de chaîne de caractère
};