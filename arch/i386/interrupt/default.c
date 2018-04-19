/*
 * This file was created on Mon Apr 16 2018
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
#include <types.h>
#include <const.h>
#include <i386/cpu.h>
#include <lib/stdio.h>
#include <driver/vga/text.h>
#include <driver/bochs/bochs.h>

/**
 * @brief Cette fonction capte les interruptions non prévu et stop le système par sécurité
 * (em mode debug uniquement) et pour me forcer à résoudre le problème :)
 * 
 * @param (unused) state: L'état des registres avant l'interruption
 */
void default_handler(struct cpu_user_state _unused *state)
{
#ifdef DEBUG_MODE
	panic("Interruption non programmée");
#endif

	//TODO: Logguer cette action
}