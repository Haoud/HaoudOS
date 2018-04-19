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
#include <core/fs/super.h>
#include <core/fs/inode.h>

struct ramdisk_super
{
	struct super_block super;						// Super Block de "base" integr� au super block du ramdisk					
	struct ramdisk_inode *inode_list;				// Liste /!\ compl�te /!\ des inodes de ce super block
};

struct ramdisk_inode
{
	struct inode inode_super;						// Inode de "base" integr� au inode du ramdisk

	struct
	{
		size_t file_size;						// Taille de donn�es
		size_t allocated_size;					// Taille r�ellement allou�
		void *data_ptr;							// Pointeur sur les donn�es
	}file;

	struct
	{
		struct ramdisk_dirent *entries;			// Liste des entr�es du r�pertoire
		count_t top_creat_order;				// Pour �viter des probl�mes de d�synchronisation
	}dir;

	struct ramdisk_inode *prev;
	struct ramdisk_inode *next;
};

struct ramdisk_dirent
{
	char *name;							// Nom de l'entr�e du r�pertoire
	struct inode *inode;				// Inode associ� � l'inode
	count_t creat_order;				// Eviter des probl�mes de d�synchronisation

	struct ramdisk_dirent *sibling_prev;
	struct ramdisk_dirent *sibling_next;
};

void RegisterRamdisk(void);