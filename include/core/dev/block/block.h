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

struct blockdev_ops
{
    hret_t (*read_block)(void *custom_data, void *dst_buf, off_t offset);
    hret_t (*write_block)(void *custom_data, void *src_buf, off_t offset);
    hret_t (*ioctl)(void *custom_data, uint32_t request_id, uint32_t request_arg);
};

struct blockdev
{
    uint64_t id;                        // Identificateur unique
    count_t count;                      // Nombre de blocs
    size_t block_size;                  // Taille des blocs
    uint32_t block_num;                 // Nombre de blocs que contient le périphérique
    struct blockdev_ops *ops;           // Opérations sur les périphériques block

    /* Pour les partitions seulement */    
    struct blockdev *parent;
    off_t first_block;  

    dev_t dev_id;
    void *private_data;

    struct blockdev *prev;
    struct blockdev *next;
};

hret_t init_blockdev(void);
hret_t unregister_device(dev_t dev_id);
hret_t blockdev_ref_by_id(dev_t dev_id);
hret_t blockdev_ref(struct blockdev *dev);
hret_t blockdev_unref(struct blockdev *dev);
struct blockdev *lookup_blockdev(dev_t devid);
hret_t blockdev_release(struct blockdev *blockdev);
hret_t register_blockdev(dev_t dev_id, size_t block_size, count_t nb_blocks, struct blockdev_ops *ops, void *custom_data);
hret_t register_partition(dev_t dev_id, struct blockdev *parent, count_t number_of_block, off_t first_block_index, void *custom_data);

hret_t blockdev_kernel_read(struct blockdev *to_read, off_t offset, void *dst_buf, size_t *len);
hret_t blockdev_kernel_write(struct blockdev *to_write, off_t offset, void *src_buf, size_t *len);

hret_t blockdev_ref_new_inode(struct inode *i);
hret_t blockdev_release_inode(struct inode *i);

bool_t blkdev_present(dev_t id);