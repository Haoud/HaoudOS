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
#include <core/process/wait.h>

#define TTY_BUFFER_SIZE		128

/* Commande ioctl pour le terminal */
#define TTY_SET_FLAG			1
#define TTY_CLEAR_FLAG			2
#define TTY_RESTORE_DEFAULT		3
#define TTY_GET_FLAG			4
#define TTY_SET_TIMEOUT			5

#define TTY_ECHO				0x01
#define TTY_CANONIC_MODE		0x02

#define TTY_DEFAULT_PARAMS		(TTY_CANONIC_MODE | TTY_ECHO)

struct tty_device
{
	minor_t minor;						// Nombre mineur du terminal
	count_t count;						// Nombre de fois que le terminal est utilisé
	flags_t params;						// Options du terminal (echo, mode canonique...)
	uint32_t timeout;					// Temps avant timeout de read (mode brute), 0 = pas de timeout

	struct tty_buffer *raw;
	struct tty_buffer *cooked;
										// Sémaphore (1 seul processus à la fois peut lire ou écrire)
	struct wait_queue *wait_on_buffer;	// Processus en attente de lecture ou d'écriture
	struct wait_queue *wait_on_enter;	// Prcessus qui attend un retour à ligne pour recevoir leurs caractères

	hret_t(*write)(const struct tty_device *tty, char c);				// Fonction qui permet d'afficher la sortie du terminal (écran, port série etc)

	struct tty_device *prev;
	struct tty_device *next;
};

struct tty_buffer
{
	int read_off;						// Pointeur vers les données à lire
	int write_off;						// Pointeur vers les données à écrire
	char buffer[TTY_BUFFER_SIZE];			// Buffer de lecture et d'écriture
};

#define init_tty_buffer(buf)		\
	(buf)->read_off = 0;			\
	(buf)->write_off = 0;

#define tty_buffer_empty(buf)		((buf)->read_off == (buf)->write_off)
#define tty_buffer_left(buf)		(((buf)->write_off - ((buf)->read_off - 1)) % TTY_BUFFER_SIZE)
#define tty_buffer_full(buf)		(!tty_buffer_left(buf))

char tty_buffer_getc(struct tty_buffer *tty_buf);
hret_t tty_buffer_putc(struct tty_buffer *tty_buf, char c);


hret_t tty_init(void);
hret_t tty_remove(struct tty_device *tty);
hret_t tty_add_char(struct tty_device *tty, char c);		// Ne doit pas être utilisé par le noyau
hret_t tty_add_chars(struct tty_device *tty, char *s);
hret_t tty_create(minor_t tty_minor, hret_t(*write)(const struct tty_device *tty, char c), struct tty_device **tty_result);

struct tty_device *lookup_tty(minor_t tty_minor);