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

#define PTR_SIZE			    4				// Un pointeur prend 4 octet de mémoire dans tout les cas

typedef unsigned char		    uint8_t;
typedef unsigned short		    uint16_t;
typedef unsigned long		    uint32_t;
typedef unsigned long long	    uint64_t;

typedef signed char			    int8_t;
typedef signed short		    int16_t;
typedef signed long			    int32_t;
typedef signed long long	    int64_t;

typedef signed long			    hret_t;			// Type de retoure par défaut
typedef signed long			    bool_t;			// Booléan
typedef unsigned int		    ptr_t;			// Faux pointeur
typedef unsigned int		    mode_t;			// Mode d'ouverture
typedef unsigned int		    size_t;			// Taille
typedef unsigned int		    flags_t;		// Flags
typedef unsigned int		    count_t;		// Compteur
typedef signed long long        off_t;			// Offset
typedef unsigned int		    time_t;			// Temps (peu importe l'unité: tick/seconde/minute...)

#define set_bits(var, mask)		((var) |= (mask))
#define clear_bits(var, mask)	((var) &= ~(mask))
#define test_bit(var, mask)		((var) & (mask))

#define O		8				// Un octet = 8 bits
#define KO		O * 1024		// Un kilo-octet = 8192 bits = 1024 octets
#define MO		KO * 1024
#define GO		MO * 1024
#define TO		GO * 1024

#define FALSE	0
#define TRUE	1

#define NULL ((void *)0)

#define RET_OK				0
#define ERR_UNKNOW			1				// Erreur inconnue
#define ERR_BUSY			2				// Ressource occupée
#define ERR_NOT_FOUND		3				// Ressource non trouvée
#define ERR_BAD_ARG			4
#define ERR_PERM			5
#define ERR_NO_DIR			6
#define ERR_NO_ENTRY		7
#define ERR_ACCES			8
#define ERR_ALREADY_EXIST	9
#define ERR_NO_MEM			10
#define ERR_NOT_IMPL		11
#define ERR_NOT_DIR			12
#define ERR_NOT_FILE		13
#define ERR_NOT_EMPTY		14
#define ERR_TOO_BIG			15
#define ERR_NO_DEV			16
#define ERR_NO_SYS			17
#define ERR_RANGE			18				// Le buffer est trop petit
#define ERR_IO  			19			        
#define ERR_NO_CHILD        20

#define _unused     __attribute__((unused))
#define _packed     __attribute__((packed))	