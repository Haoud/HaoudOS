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

#define SHELL_NR_CMD	5
#define SHELL_CMD ">"
#define clear_buf(buf, size)		memset(buf, 0, size);

typedef void(*cmd_callback)(char *);

struct cmd_struct
{
	char *cmd_name;
	char *description;
	cmd_callback cmd_fn;
};

void start_shell(void);
void shell_init(void);
void shell_loop(void);
void input_line(char *buf, size_t max_char);

void _exit(char *buf);
void echo(char *buf);
void help(char *buf);
void reboot(char *buf);
void bprint(char *buf);