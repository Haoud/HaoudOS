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
#include <core/fs/super.h>
#include <core/process/process.h>

enum inode_type
{
	INODE_FILE = 1,
	INODE_DIRECTORY = 2,
	INODE_CHAR_DEVICE = 3,
	INODE_BLOCK_DEVICE = 4,
	INODE_SYMBOLIC_LINK = 5,
	INODE_PIPE = 6,
	INODE_FIFO = 7
};

struct inode
{		
	struct super_block *super;				// Super Block associé à l'inode
	enum inode_type type;					// Type de l'inode (fichier, répertoire, lien symbolique...)
	count_t memory_count;					// Nombre de référence à l'inode en mémoire
	count_t disk_count;						// Nombre de référence à l'inode sur le disque
	count_t generation;						// Pour syncroniser l'inode avec le descripteur de fichier
	void *private_data;						// Données spécifique du driver du fs qui gère cet inode
	dev_t dev;								// Périphériques bloc ou caractère uniquement: identificateur unique
	uint64_t inode_id;						// Identificateur unique (dans le super block) de l'inode
	bool_t is_dirty;						// Booléean indiquant si l'inode est modifié
									
	hret_t(*destructor)(struct inode *to_del);
	hret_t(*close_file)(struct inode *this_inode, struct file *to_close);
	hret_t(*open_file)(struct inode *this_inode, struct process *owner, flags_t open_flags, struct file **result);

	struct inode_operation *inode_op;		// Communs à tous les inodes
	struct inode_dir_operation *dir_op;		// Uniquement si l'inode est un répertoire

	struct inode *used_prev;
	struct inode *used_next;

	struct inode *dirty_prev;
	struct inode *dirty_next;
};

struct stat
{
	dev_t dev;         	// Identificateur du périphérique ou se trouve l'inode
	uint64_t ino;     	// Numéro d'inode
	uint32_t type;     	// Type de fiche et mode
	count_t nlink;     	// Nombre de lien sur le disque
	dev_t rdev;        	// Identificateur de périphérique (inode char ou block)
	off_t size;        	// Taille total en octet
	uint64_t blksize;  	// Taille des blocks du system de fichier
}_packed;

struct inode_operation
{
	hret_t (*stat)(struct inode * this_inode, struct stat *result);
	hret_t(*truncate)(struct inode *this_inode, off_t length);
	hret_t(*sync)(struct inode *this_inode);
};

struct inode_dir_operation
{
	hret_t(*lookup)(struct inode *this_inode, const char *name, uint64_t *result_inode_id);
	hret_t(*link)(struct inode *this_inode, const struct process *actor, const char *name, struct inode *inode_to_link);
	hret_t(*unlink)(struct inode *this_inode, struct process *actor, const char *name);
};

void ref_inode(struct inode *inode);
void unref_inode(struct inode *inode);
void sync_inode(struct inode *to_sync);
void mark_inode_as_dirty(struct inode *to_mark);
void register_root_inode(struct super_block *super, struct inode *root);
hret_t lookup_parent(const char *_path, char **remaning_path, struct inode **result);
hret_t lookup_inode(const char *_path, char **remaning_path, struct inode **result);
hret_t fetch_inode(struct super_block *super, uint64_t inode_id, struct inode **result);
hret_t create_inode(const char *_path, const struct process *actor, enum inode_type type);
hret_t allocate_inode(struct super_block *super, enum inode_type type, struct inode **result);
hret_t make_speacial_node(const char *_path, const struct process *actor, enum inode_type type, dev_t devid);
hret_t connext_existing_child_inode(const struct process *actor, struct inode *parent, const char *name, struct inode *to_register);
hret_t create_child_inode(const struct process *actor, struct inode *parent, const char *name, enum inode_type type, uint32_t flags);
hret_t register_child_inode(const struct process *actor, struct inode *parent, const char *name, struct inode *to_register, uint32_t flags);
