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
#include <core/dev/dev.h>
#include <core/fs/file.h>
#include <core/fs/inode.h>

struct chardev
{
	major_t major;					// Nombre qui sert d'identifiant à un type de périphérique caractère
	uint32_t count;					// Nombre de fois que cette classe est utilisée
	void *private_data;				// Données privées

	struct chardev_ops *ops;		// Opérations du VFS correspondante
	struct chardev *prev;
	struct chardev *next;
};

struct chardev_ops
{
	// Fonction open (obligatoire)
	hret_t(*open) (struct inode *opened_inode, struct file *opened_file, void *private_data);

	// Fonction close
	hret_t(*close) (struct file *opened_file, void *private_data);

	// Fonction seek
	hret_t(*seek) (struct file *open_file, seek_t whence, off_t offset, off_t *result);
	
	// Fonction read
	hret_t(*read) (struct file *open_file, char *dst_buf, size_t *len);

	// Fonction write
	hret_t(*write) (struct file *open_file, char *src_buf, size_t *len);

	// Fonction fnctl
	hret_t(*fcntl) (struct file *open_file, uint32_t request_id, uint32_t request_arg);

	// Fonction ioctl
	hret_t(*ioctl) (struct file *open_file, uint32_t request_id, uint32_t request_arg);

};

struct chardev_openfile
{
	struct file super;				// Structure d'un fichier ouvert par défaut
	struct chardev *device;			// Périphérique caractère associé à fichier ouvert
};

hret_t register_major_chardev(major_t major, struct chardev_ops *ops, void *custom_data);
hret_t unregister_major_chardev(major_t major);
struct chardev *lookup_chardev(major_t devid);

/* 
 * Fonctions appelés lorsque le VFS référence ou libère un inode afin que les périphériques
 * caractère soit informés de ce changement 
 */
hret_t chardev_ref_new_inode(struct inode *i);
hret_t chardev_release_inode(struct inode *i);				// Cette fonction ne sert à rien pour l'instant