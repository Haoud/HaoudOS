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
#include <i386/tsc.h>
#include <lib/stdio.h>

static bool_t tsc_present = FALSE;

/**
 * @brief 
 * 
 */
void DetectTSC(void)
{
	struct cpuid_result res;

	// Si l'instruction CPUID est supportée
	if (cpuid(CPUID_CMD_GET_FEATURES, &res))
	{
		// Est ce que le processeur possède un TSC ?
		if (res.edx & CPUID_FEATURE_TSC_PRESENT)
			tsc_present = TRUE;
		else
			tsc_present = FALSE;

		if (tsc_present)
			debugk("[TSC]: TSC found on your system\n");
		else
			debugk("[TSC]: No TSC found on your system\n");

		return;
	}

	tsc_present = FALSE;
	debugk("[TSC]: CPUID not supported, unable to detect if a TSC is present\n");
	return;
}

/**
 * @brief Permet de lire la valeur du compteur du TSC
 * 
 * @return uint64_t la valeur du compteur du TSC si le PC possède un TSC, 0 sinon
 */
uint64_t ReadTSC(void)
{
	uint64_t val = 0;
	
	if (tsc_present)
		asm("rdtsc" :"=A"(val) :: "memory");

	// Si le processeur n'a pas de TSC on retourne 0 (erreur)
	return val;
}