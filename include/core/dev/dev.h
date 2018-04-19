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

#define GET_MAJOR(dev)			((major_t)((dev & 0xFFFF0000) >> 16))
#define SET_MAJOR(dev, value)	(clear_bits(dev, 0xFFFF0000);					\
								 set_bits(dev, ((value & 0xFFFF) << 16)))

#define GET_MINOR(dev)			((minor_t)(dev & 0x0000FFFF))
#define SET_MINOR(dev, value)	(clear_bits(dev, 0x0000FFFF);					\
								 set_bits(dev, (value & 0xFFFF)))

#define MAKE_DEV(major, minor)	(((major) << 16) + (minor))

// Utiliser par mknod (appel système)
#define S_IFREG		0x01
#define S_IFBLK		0x02
#define S_IFDIR		0x04
#define S_IFCHR		0x08

typedef unsigned int dev_t;
typedef unsigned short major_t;
typedef unsigned short minor_t;

// Nombre majeurs des périphériques caractères et bloc
#define TTY_MAJOR			1
#define TTY_DEFAULT_MINOR	0

#define HDD_MAJOR			3