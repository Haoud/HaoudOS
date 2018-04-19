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
#include <core/mm/phys.h>
#include <core/fs/file.h>
#include <core/fs/inode.h>

#define pipe_empty(p)		((p)->read_off == (p)->write_off)
#define pipe_left(p)		(((p)->write_off - ((p)->read_off - 1)) % PAGE_SIZE)
#define pipe_full(p)		(!pipe_left(p))


struct pipe_info
{
    char *pipe_data;
    unsigned int read_off;              // Position de lecture dans le buffer
    unsigned int write_off;             // Position d'�criture dans le buffer

    unsigned int writers;               // Nombre d'�crivain
    unsigned int readers;               // Nombre de lecteur
    struct semaphore *atomic_write;
    struct wait_queue *standby_readers;
    struct wait_queue *standby_writers;
};

struct inode *get_pipe_inode(void);
hret_t make_pipe(int mode, struct file *fd, struct inode *pipe_inode);