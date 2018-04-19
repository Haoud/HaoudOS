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
#include <core/fs/file_system.h>

#define MAX_MOUNT			26			// Lettre de "A" à "Z"

//enum inode_type;						// Pour supprimer un avertissement

enum super_flags
{
	SUPER_MOUNT_SYNC = 0x01,
	SUPER_MOUNT_NO_EXEC = 0x02,
	SUPER_MOUNT_READ_ONLY = 0x04,
};

struct super_block
{
	struct file_system *fs;				// Système de fichier associé au super_block
	enum super_flags flags;				// Options de montage de ce super block
			
	struct inode *dirty_list;			// Liste des inodes modifiés
	struct inode *used_list;			// Liste des inodes en mémoire
	struct inode *root;					// Inode racine
	dev_t dev;							// Permet de lire ou écrire sur un périphérique bloc

	void *custom_data;					// Librement utilisable par le système de fichier
	struct super_block *prev;			// Précédent super block partagant le même système de fichier
	struct super_block *next;			// Prochain super block partagant le même système de fichier
	struct super_operation *s_op;		// Opérations du super block
};

struct super_operation
{
	hret_t(*fetch_inode)(struct super_block *this_super, uint64_t inode_id, struct inode **result);
	hret_t(*allocate_inode)(struct super_block *this_block, enum inode_type type, struct inode **result);
};

struct super_block *get_super(char drive);
void add_super(struct super_block *super, char drive_letter);
void remove_super(char drive_letter);
void init_mount_list(void);