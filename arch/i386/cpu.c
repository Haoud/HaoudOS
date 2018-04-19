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
#include <i386/cpu.h>
#include <lib/stdio.h>

static bool_t support_cpuid = FALSE;


/**
 * @brief Cette fonction essaye de d�t�cter si l'instruction CPUID est support� car tr�s  
 * utilis� pour la d�t�ction de mat�riel interne au CPU (FPU, TSC...)
 */
void cpuid_init(void)
{
	uint32_t current_eflags;					// Configuration d'origine d'EFLAGS
	uint32_t modified_elflags;					// EFLAGS qui sera modifi�

	get_eflags(current_eflags);					// On obtient le registre EFLAGS et on le sauvegarde dans current_eflags
	modified_elflags = current_eflags;			// On reprendre la m�me configuration que d'origine mais...
	modified_elflags ^= EFLAGS_ID;				// On inverse le bit ID

	set_eflags(modified_elflags);				// On modifie EFLAGS
	get_eflags(modified_elflags);				// On r�cup�re EFLAGS pour voir le r�sultat

	/*
	 * Si on peut changer le bit ID du registre EFLAGS librement, c'est que l'instruction CPUID est
	 * support� par le processeur. Sinon, c'est que le processeur est tr�s ancien et qu'il ne le
	 * supporte pas.
	 * Mais certains vieux processeurs supporte CPUID mais ne permette pas le changement du bit ID
	 * du registre EFLAGS, il faudrai donc trouver une autre technique
	 */

	if (modified_elflags == current_eflags)
		support_cpuid = FALSE;
	else
		support_cpuid = TRUE;

	set_eflags(current_eflags);					// On restaure l'ancien EFLAGS pour �viter tout probl�me
	
	if (support_cpuid)
		debugk("[CPU]: CPUID instruction are supported\n");
	else
		debugk("[CPU]: CPUID instruction aren't supported!! This isn't good...\n");
}

/**
 * @brief Cette fonction permet d'utiliser simplement l'instruction CPUID 
 * 
 * @param cmd La commande CPUID qu'il faut executer
 * @param res La structure qui va contenir le resultat de la commande
 * @return bool_t TRUE si l'instruction cpuid a �t� execut� (correctement ou non), FALSE sinon
 */
bool_t cpuid(uint32_t cmd, struct cpuid_result *res)
{
	if (!support_cpuid)
		return FALSE;
			
	asm volatile("cpuid" : "=a"(res->eax), "=b"(res->ebx), "=c"(res->ecx), "=d"(res->edx) : "a"(cmd) : "memory");
	return TRUE;
}
