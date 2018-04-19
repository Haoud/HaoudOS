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
#include <const.h>
#include <lib/stdio.h>
#include <lib/stdlib.h>
#include <lib/stdarg.h>
#include <core/fs/vfs.h>
#include <core/mm/phys.h>
#include <i386/ioports.h>
#include <lib/vsnprintf.h>
#include <driver/vga/text.h>

char console_buffer[80 * 25 * 2];


const char *crash_logo =
"                             ._________________.\n"
"                             | _______________ |\n"
"                             | I             I |\n"
"                             | I    OOPS !   I |\n"
"                             | I HaoudOS has I |\n"
"                             | I   crashed   I |\n"
"                             | I_____________I |\n"
"                             !_________________!\n"
"                                ._[_______]_.\n"
"                            .___|___________|___.\n"
"                            |::::::::           |\n"
"                            |:::::::: [HaoudOS] |\n"
"                            !___________________!\n";

void panic(const char *fmt, ...)
{
	char print_buffer[128];
	int scancode;
	va_list arg;
	int port;

	// TODO: vérifier si c'est possible
	do_sync();

	va_start(arg, fmt);
	vsnprintf(print_buffer, 128, fmt, arg);
	va_end(arg);

	SaveConsole(console_buffer);

	ClearScreenColor(LIGHT_GREY, BLUE);
	SetTextColor(LIGHT_GREY, BLUE);

	Print(crash_logo);
	Print("\n");

	// Les mots "données" et "et" sont collés, c'est normal...					      ----vv----
	Print("HaoudOS a crashé et ne peut continuer son éxécution afin de protéger vos donnéeset votre matériel. Merci de contacter le développeur si ce problème se reproduit\n");
	
	printk("Message d'erreur: %s\n\n", print_buffer);
	printk("ENTER: Redémarrer l'ordinateur\n");
	printk("TAB: Revisualiser la console avant le crash\n");
#ifdef DEBUG_MODE
	printk("ECHAP: Outrepasser le crash (Peut corrompre toutes vos données)\n");
#endif

	for (;;)
	{
		port = 0;
		while (!(port & 0x01))			// Attend qu'une touche soit pressée
			port = inb(0x64);

		// Lecture du scancode de cette touche
		scancode = inb(0x60);

		if (scancode == 28)				// Touche "Entrée" pressé
			hard_reboot();				// Redémarrage de l'ordinateur
		else if (scancode == 15)		// Tabulation
			memcpy((void *)0xB8000, console_buffer, 80 * 25 * 2);			// Note: pas modulaire ni compatible avec le driver vga d'HaoudOS
#ifdef DEBUG_MODE
		else if (scancode == 1)
		{
			memcpy((void *)0xB8000, console_buffer, 80 * 25 * 2);
			SetTextColor(LIGHT_GREY, BLACK);						// Peut ne pas être exact
			return;
		}
#endif
	}
}
