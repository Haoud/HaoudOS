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
#include <const.h>
#include <core/fs/super.h>

void PrepareVFS(void);
void SetupVFS(const char *fs_name, const char *root_device, char mountpoint);

void do_sync(void);
void sync_all_fs(void);
void sync_fs(struct super_block *to_sync);

char *get_last_name(const char *path);
hret_t parse_path(const char *path, char **parsed_path);

hret_t kernel_create_file(const char *pathname);
hret_t kernel_write_file(const char *pathname, const char *buffer, off_t offset, size_t len);

void vfs_debugk(const char *sys, const char *fmt, ...);