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
#include <types.h>

struct Tss
{
	uint16_t __reserved_link, link;
	uint32_t esp0;
	uint16_t ss0, __ss0_reserved;
	uint32_t esp1;
	uint16_t ss1, __ss1_reserved;
	uint32_t esp2;
	uint16_t ss2, __ss2_reserved;

	uint32_t cr3;
	uint32_t eip;
	uint32_t eflags;
	uint32_t eax;
	uint32_t ecx;
	uint32_t edx;
	uint32_t ebx;
	uint32_t esp;
	uint32_t ebp;
	uint32_t esi;
	uint32_t edi;

	uint16_t es, __es_reserved;
	uint16_t cs, __cs_reserved;
	uint16_t ss, __ss_reserved;
	uint16_t ds, __ds_reserved;
	uint16_t fs, __fs_reserved;
	uint16_t gs, __gs_reserved;
	uint16_t ldt, __ldt_reserved;
	uint32_t debug, io_map;
}_packed;

extern struct Tss tss_kernel;

void InstallTss(void);
void SetupTss(void);
void FlushTss(void);
