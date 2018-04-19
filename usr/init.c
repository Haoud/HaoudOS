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
#include <core/dev/dev.h>
#include <usr/libc/stdio.h>
#include <usr/libc/stdlib.h>
#include <core/process/process.h>

void init(void)
{
	const char *shell_argv[] = {"shell", NULL};
	const char *shell_env[] = {"PATH=/bin:/usr/bin:", "HAOUD_PATH=C:\\bin\\;C:\\usr\\bin\\;", NULL};
	int status = 0;
	int ret = 0;

	/* Ouvre stdin/stdout/stderr */
	ret |= open("C:\\dev\\tty", O_RDONLY);
	ret |= open("C:\\dev\\tty", O_WRONLY);
	ret |= open("C:\\dev\\tty", O_WRONLY);

	if(ret == -1)										// Si impossible d'ouvrir les flux standards
		exit(-1);										// On quitte init avec une erreur
	
	if(fork() == 0)
		execve("shell", shell_argv, shell_env);			// Exécute le shell

	for(;;)												// Adopte les processus orphelins et attend qu'ils meurent
		(void)wait(&status);							// Attend un enfant mort

	exit(-1);											// N'est jamais executé
}