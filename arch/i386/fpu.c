/*
 * This file was created on Tue Mar 27 2018
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
#include <i386/fpu.h>
#include <i386/cpu.h>
#include <i386/idt.h>
#include <lib/stdio.h>
#include <core/process/process.h>

extern void asm_device_not_avaible(void);


/**
 * @brief Permet d'initialiser le FPU. Après l'appel à cette fonction, le FPU peut être
 * utilisé par le noyau
 * 
 * TODO: Implémenter une detection du FPU et des instructions étendues (MMX, SSE...)
 */
void FpuSetup(void)
{
	debugk("[FPU] FPU detected\n");

	MakeIdtDescriptor(DEVICE_NOT_AVAIBLE_INT_NR, (uint32_t)asm_device_not_avaible, 0x08, IDT_INT_GATE32 | IDT_HANDLER_PRESENT);

	set_ts();		// Indique que le FPU n'est pas disponible
	return;
}

/**
 * @brief Cette fonction permet de sauvegarder les registres en virgule flottante
 * du processus en cours: ils sont stocké dans la structure du processus 
 */
void save_fpu_state(void)
{
	if(current->fpu_state_loaded)
	{
		asm volatile("fnsave %0" : "=m"(current->fpu_contexte) :: "memory");
		asm volatile("fwait");

		current->fpu_state_loaded = FALSE;
	}

	set_ts();
}

/**
 * @brief Permet de restorer (ou d'initialiser) les registres en virgule flottante du
 * processus courant
 */
void restore_fpu_state(void)
{
	asm volatile("clts");			// Indique que le contexte du FPU est à jour (enlève le bit TS, Task Switched)
	if(current->used_fpu)
	{
		asm volatile("frstor %0" :: "m"(current->fpu_contexte) : "memory");
	}
	else
	{
		asm volatile("finit");
		current->used_fpu = TRUE;
	}
	current->fpu_state_loaded = TRUE;
}

/**
 * @brief Cette fonction est appelé par le processeur lorsque une instruction FPU 
 * est utilisé alors que le drapeau TS est armé.
 * Elle appele juste la fonction restore_fpu_state puis se termine
 * 
 * @param regs L'état des registres du programme interrompu
 */
void device_not_avaible(struct cpu_state _unused *regs)
{
	restore_fpu_state();
}