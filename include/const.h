/*
 * This file was created on Wed Mar 28 2018
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

#define OS_NAME				"HaoudOS"
#define OS_VERSION			"0.0.01"
#define OS_AUTOR			"CADILHAC Romain"
#define OS_ARCH				"x86"
#define OS_MINIMAL_CPU		"i386+"

#define DEBUG_MODE

#ifdef DEBUG_MODE
	#define FULLY_DEBUGING_KERNEL				// Affiche plein de message de debug sur le port 0xe9 (bochs)
#endif

#ifdef FULLY_DEBUGING_KERNEL
	#define EXT2_PRINT_DEBUG            1		// Affiche les message de debug du système de fichier ext2
	#define VFS_PRINT_DEBUG             2		// Affiche les message de debug du système de fichier virtuel
	#define SYSCALL_PRINT_DEBUG         3		// Affiche les message de debug des appels systèmes
#endif

#ifndef DEBUG_MODE
	#define RELEASE_MODE
#endif