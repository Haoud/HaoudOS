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
#include <i386/cpu.h>
#include <lib/stdlib.h>
#include <i386/ioports.h>

static uint32_t rand_seed = 1;

int rand(void)
{
	rand_seed = rand_seed * 1103515245 + 12345;
	return (unsigned int)(rand_seed / 65536) % RAND_MAX;
}

void hard_reboot(void)
{
	cli();

	uint8_t good = 0x02;
	while (good & 0x02)			// Attend que le controleur du clavier soit pr�t
		good = inb(0x64);		

	outb(0x64, 0xFE);			// Red�marrage de l'ordinateur

	hlt();
}

void srand(const int seed)
{
	rand_seed = seed;
}

char cp1252_to_cp437(const char c)
{
	switch (c)
	{
		case '�':				// '�' encodage windows 1252
			return 0x82;		// '�' encodage cp 437

		case '�':				// '�' encodage windows 1252
			return 0x8A;		// '�' encodage cp 437

		case '�':				// '�' encodage windows 1252
			return 0x87;		// '�' encodage cp 437

		case '�':				// '�' encodage windows 1252
			return 0x85;		// '�' encodage cp 437

		default:
			return c;			// Caract�re compatible ou non pris en charge
	}
}

/*
 * Cette fonction permet de copier n octets de src � dst
 *
 * TODO: cette fonction n'est pas du tout optimis�, il 
 * faudrai penser � le faire
 */
void *memcpy(void *dst, const void *src, size_t len)
{
	char *cdst = dst;
	char *csrc = (char *)src;

	while (len--)
		*cdst++ = *csrc++;

	return dst;
}

/*
 * Cette fonction permet de mettre len octet � la valeur sp�cifi� en
 * arguement vers la destination: utilis� surtout pour mettre � z�ro
 * une zone m�moire
 *
 * TODO: optimiser la fonction
 */
void *memset(void *dst, int value, size_t len)
{
	char *cdst = dst;

	while (len--)
		*cdst++ = value;

	return dst;
}