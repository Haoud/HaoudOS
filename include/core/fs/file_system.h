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

struct file_system
{
	char *name;									// Nom du système de fichier concerné
	void *private_data;							// Librement utilisable par le driver du système de fichier concerné
	struct super_block *super_list;				// super block utilisant ce système de fichier
	struct file_system_operation *fs_op;		// Opérations de montage du système de fichier

	/* Liste doublement chainée */
	struct file_system *prev;
	struct file_system *next;
};

struct file_system_operation
{
	hret_t(*mount)(struct file_system *this_fs, dev_t device ,struct super_block **result_super);
	hret_t(*umount)(struct file_system *this_fs, struct super_block *to_umount);
};

void unregister_fs(const char *fs_name);
void register_fs(struct file_system *fs);
struct file_system *get_fs(const char *name);

hret_t umount(char drive);
hret_t mount(char drive, const char *fs_name, const char *device);
