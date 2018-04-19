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
#include <maths.h>
#include <lib/ctype.h>
#include <lib/string.h>
#include <lib/vsnprintf.h>
#include <driver/bochs/bochs.h>

/*
* Cette fonction, utilis�e par vsnprintf, permet de convertir
* une cha�ne de caract�re (char *) en un entier sign� (int)
*
* NOTE: cette fonction modifie le pointeur pass� en argument
* gr�ce � un pointeur de pointeur (oul�, c'est compliqu�...)
*/
int vsnprintf_atoi(const char **s)
{
	int i = 0;

	while (is_digit(**s))
		i = i * 10 + *((*s)++) - '0';

	return i;
}

int snprintf(char *buf, const size_t n, const char *format, ...)
{
	int len_writed = 0;
	va_list arg;

	va_start(arg, format);
	len_writed = vsnprintf(buf, n, format, arg);
	va_end(arg);

	return len_writed;
}

/*
 * Cette fonction permet de convertir une chaine de format en une chaine
 * de carac�re ASCII valide
 *
 * Je me suis efforc� de rendre la fonction la plus fiable et la plus proche
 * de celle de la libc, mais quelques fonctionnalit� de je juge inutile n'ont
 * pas �t� impl�ment�e, de plus, certain fonctionnalit� que je juge utile mais
 * absente de la libc (affichage binaire) ont �t� rajout�: Donc, attention si 
 * vous utilis�e cette fonction comme celle de la libc
 *
 * /!\ ATTENTION /!\ Cette est probablement bugg� � cause de la fonction 'number'
 * probablement mal cod�e (mais qui fonctionne)
 */
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough" int vsnprintf(char *buf, const unsigned int size, const char *format, va_list arg)

int vsnprintf(char *buf, const unsigned int size, const char *format, va_list arg)
{
	unsigned int size_restante = size - 1;				// Reserve de la place pour le caract�re nul
	char *buf_ptr = NULL;
	int field_size = -1;

	flags_t flags = 0;

	// Fausse boucle FOR
	for (buf_ptr = buf; *format && size_restante; format++)
	{
		if (*format != '%')
		{
			*buf_ptr = *format;

			buf_ptr++;
			size_restante--;
			continue;
		}

		flags = 0;

	next_flag:
		format++;
		switch (*format)
		{
			case '+': flags |= VSNPRINT_PLUS;		goto next_flag;
			case ' ': flags |= VSNPRINT_SPACE;		goto next_flag;
			case '0': flags |= VSNPRINT_ZEROPAD;	goto next_flag;
		}

		field_size = -1;
		if (is_digit(*format))
			field_size = vsnprintf_atoi(&format);

		switch (*format)
		{
			// Pour affiche des entier sign� ou non sign� (int) en base 10
			case 'd':
			case 'i':
				flags |= VSNPRINT_SIGN;
			case 'u':
				buf_ptr = number(buf_ptr, va_arg(arg, int), 10, field_size, &size_restante, flags);
				break;

			// Affiche un caract�re
			case 'c':
				if (size_restante--)
					*(buf_ptr++) = (char)va_arg(arg, char);
				break;

			// Affiche un chaine de caract�res
			case 's':
				for (char *str = va_arg(arg, char *); *str != '\0' && size_restante; str++, size_restante--)
					*(buf_ptr++) = *str;
				break;

			// Affiche d'un nombre en base 16
			case 'X':
				flags |= VSNPRINT_LARGE;
			case 'x':
				buf_ptr = number(buf_ptr, va_arg(arg, int), 16, field_size, &size_restante, flags);
				break;

			// Affichage d'un pointeur
			case 'p':
				buf_ptr = number(buf_ptr, va_arg(arg, int), 16, 8, &size_restante, flags | VSNPRINT_ZEROPAD);
				break;

			// Affiche d'un entier en binaire
			case 'b':
				buf_ptr = number(buf_ptr, va_arg(arg, int), 2, field_size, &size_restante, flags);
				break;

			// Si l'utilisateur veut vraiment afficher %
			case '%':
				if (size_restante--)
					*(buf_ptr++) = '%';
				break;

			default:
				break;
		}
	}

	*buf_ptr = 0;
	return buf_ptr - buf;
}

/*
 * Cette fonction, utilis�e par vsnprintf, permet de convertir un nombre (int)
 * en un r�persentation en ASCII (au choix binaire, h�xa...) et permet quelques
 * options gr�ce � l'argument 'options'.
 *
 * NOTE: Cette fonction est probablement tr�s mal �crite et tr�s, tr�s buggu�,
 * surtout au niveau du buffer overflow. Si possible, ne vous fiez pas � cette
 * fonction probablement tr�s dangeureuse
 */
char *number(char *str, int num, int base, int size, unsigned int *max_size, flags_t options)
{
	const char *convert_table = "0123456789abcdef";
	char sign, padding, tmp[36];
	int chiffre = 0, i = 0, msize = 0, num_max = 0;
	unsigned int tmp_num;

	msize = *max_size;
	num_max = size;

	if (options & VSNPRINT_LARGE)
		convert_table = "0123456789ABCDEF";

	if (base < 2 || base > 16)
		base = 10;		// Par d�faut pour �viter des BUGS

	if (options & VSNPRINT_ZEROPAD)
		padding = '0';
	else
		padding = ' ';

	sign = 0;
	if (options & VSNPRINT_SIGN)
	{
		if (num < 0)
		{
			sign = '-';
			num = -num;
			size--;
		}
		else if (options & VSNPRINT_PLUS)
		{
			sign = '+';
			size--;
		}
		else if (options & VSNPRINT_SPACE)
		{
			sign = ' ';
			size--;
		}
	}

	if (num == 0)
	{
		tmp[i] = '0';
		i++;
	}
	else
	{
		while (num != 0 && num_max--)
		{
			chiffre = ((unsigned int)num) % base;
			tmp[i++] = convert_table[chiffre];

			tmp_num = num;
			tmp_num /= base;
			num = tmp_num;
		}
	}

	size -= i;
	if (sign && msize)
		tmp[i++] = sign;

	// Padding avec le bon caract�re
	while (size-- > 0 && msize--)
		*(str++) = padding; 

	// Recopie dans le buffer de destination
	while (i-- > 0 && msize--)
		*str++ = tmp[i];

	*max_size = msize;						// Actualise la taille possible restante
	return str;								// Retourne la cha�ne de caract�re restante
}