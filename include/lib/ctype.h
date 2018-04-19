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

#define is_digit(c)		((c) >= '0' && (c) <= '9')
#define is_ascii(c)		(c > 0 && c < 256)
#define is_upper(c)		(c >= 'A' && c <= 'Z')
#define is_lower(c)		(c >= 'a' && c <= 'z')
#define is_letter(c)	((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))

#define to_upper(c)				\
	if(is_lower(c))				\
	{							\
		c += ('a' - 'A');		\
	}

#define to_lower(c)				\
	if (is_upper(c))			\
	{							\
		c -= ('a' - 'A');		\
	}
