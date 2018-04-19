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

struct dirent;
struct file
{
	struct process *owner;			// Processus ayant ouvert le fichier
	struct inode *inode;			// Inode associé au descripteur de fichier
	uint32_t open_flags;			// Option d'ouverture du fichier
	count_t generation;				// Pour éviter la désyncronisation avec l'inode
	off_t offset;					// Offset dans le fichier
	count_t ref;					// Nombre de référence au descripteur de fichier

	hret_t(*duplicate)(struct file *this_file, struct process *for_process, struct file **result);
	struct open_file_operation *file_op;		// Commun pour tout les inodes (toutes les fonctions ne sont pas obligatoirement présente)
	struct open_blockdev_ops *block_op;			// Uniquement si l'inode est un périphérique block	
	struct open_dir_operation *dir_op;			// Uniquement si l'inode est un dossier
	struct open_chardev_ops *char_op;			// Uniquement si l'inode est un périphérique caractère

	void *custom_data;
};

#define SEEK_SET	0				// Position à partir du début du fichier
#define SEEK_CUR	1				// Position à partir de la position courante
#define SEEK_END	2				// Position à partir de la fin du fichier

#define OPEN_READ			0x0
#define OPEN_WRITE			0x1
#define OPEN_RDWR			0x2
#define OPEN_CREAT			0x200
#define OPEN_TRUNCATE		0x400
#define OPEN_EXCL			0x800
#define OPEN_DIRECTORY		0x10000

typedef uint32_t seek_t;

#define is_valid_fd(fd)		(fd >= 0 && fd <= 31)

struct open_file_operation
{
	hret_t(*seek)(struct file *this_file, seek_t whence, off_t offset, off_t *result);
	hret_t(*fnctl)(struct file *this_file, uint32_t request_id, uint32_t request_arg);
	hret_t(*write)(struct file *this_file, char *src_buf, size_t *len);
	hret_t(*read)(struct file *this_file, char *dst_buf, size_t *len);
};

struct open_dir_operation
{
	hret_t(*readdir)(struct file *this_file, struct dirent *result);
};

struct open_chardev_ops
{
	hret_t(*ioctl) (struct file *open_file, uint32_t request_id, uint32_t request_arg);
};

struct open_blockdev_ops
{
	hret_t(*ioctl) (struct file *open_file, uint32_t request_id, uint32_t request_arg);
};

hret_t ref_open_file(struct file *f);
hret_t unref_open_file(struct file *f);
hret_t init_fd_struct(struct process *p);
size_t get_open_file_size(struct file *f);
hret_t unregister_open_file(struct process *p, int fd_index);
hret_t found_free_process_fd(struct process *p, int *result_index);
hret_t duplicate_fd_struct(struct process *modele, struct process *copieur);
hret_t get_open_file(struct process *p, uint32_t fd_index, struct file **result);
hret_t register_open_file(struct process *p, struct file *f, uint32_t *fd_index_result);
hret_t duplicate_open_file(struct file *fd, struct process *for_process, struct file **result);
hret_t new_opened_file(struct process *owner, struct inode *inode, flags_t open_flags, struct file **result);
