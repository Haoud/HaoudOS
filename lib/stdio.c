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
#include <lib/stdarg.h>
#include <lib/vsnprintf.h>
#include <driver/vga/text.h>
#include <driver/bochs/bochs.h>

int printk(const char *fmt, ...)
{
	char print_buffer[256];
	int print_len;
	va_list arg;

	va_start(arg, fmt);
	print_len = vsnprintf(print_buffer, 256, fmt, arg);
	va_end(arg);

	Print(print_buffer);
	return print_len;
}

#ifdef FULLY_DEBUGING_KERNEL
void debugk(const char *format, ...)
#else
void debugk(const char _unused *format, ...)
#endif
{
#ifdef FULLY_DEBUGING_KERNEL

	char PrintBuffer[256];
	va_list arg;

	va_start(arg, format);
	vsnprintf(PrintBuffer, 256, format, arg);
	va_end(arg);

	BochsPrint(PrintBuffer);
#endif
}