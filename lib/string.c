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
#include <lib/ctype.h>
#include <lib/string.h>

int ctoi(char c)
{
	if (is_digit(c))
		return (int)(c - '0');

	return -1;
}

int atoi(char *s)
{
	int i = 0;

	while (is_digit(*s))
		i = i * 10 + ((*s++) - '0');

	return i;
}

int strlen(const char *str)
{
	int i = 0;
	while (*(str++))
		i++;

	return i;
}		

char *strchr(const char *s, char c)
{
	for(unsigned int i = 0; s[i] != '\0'; i++)
	{
		if(s[i] == c)
			return (char *)((uint32_t)s + i);
	}
	return NULL;
}

int strcmp(const char *s1, const char *s2)
{
	while (*s1 != '\0' && *s1 == *s2)
	{
		s1++;
		s2++;
	}

	return *(s1) - *(s2);
}

int strncmp(const char *s1, const char *s2, size_t len)
{
	if (len == 0)
		return 0;
	do {
		if (*s1 != *s2++)
			return (*(unsigned char *)s1 - *(unsigned char *)--s2);
		if (*s1++ == 0)
			break;
	} while (--len != 0);

	return 0;
}

char *strcpy(char *dst, const char *src)
{
	int i;

	for (i = 0; src[i] != '\0'; i++)
		dst[i] = src[i];

	dst[i] = '\0';		// Caractère de fin de chaîne
	return dst;			// Retourne dst
}

char *strcat(char *dst, const char *src)
{
	return strcpy(dst + strlen(dst), src);
}

int strrpl(char *s1, char c1, char c2)
{
	char *s1ptr = s1;
	int n = 0;

	while(*s1ptr != '\0')
	{
		if(*s1ptr == c1)
		{
			*s1ptr = c2;
			n++;
		}

		s1ptr++;		
	}

	return n;
}