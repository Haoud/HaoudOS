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
#include <i386/cpu.h>

#define PIT_8254_MAX_FRENQUENCY			1193180		// Fréquence maximum de l'occilateur interne du PIT
#define PIT_8254_DEFAULT_FRENQUENCY		100			// Fréquence par défaut du PIT en Hz
#define PIT_8254_LATCH                  (PIT_8254_MAX_FRENQUENCY / PIT_8254_DEFAULT_FRENQUENCY)

#define PIT_8254_IDT_VECTOR		0x20				// Vecteur d'interruption de cette IRQ dans l'IDT
#define PIT_8254_IRQ_NR			0					// Numéro de cette IRQ

#define PIT_8254_TIMER0		0x40
#define PIT_8254_TIMER1		0x41
#define PIT_8254_TIMER2		0x42
#define PIT_8254_CMD		0x43

time_t get_startup_sec(void);						// Retourne le nombre de secondes écoulées depuis le démarrage du système
time_t get_tick_in_this_sec(void);					// Retourne le nombre de tick de la seconde courante
void i8254_SetFrenquency(unsigned int freq);		// Appelé uniquement par la fonction d'initialisation d'HaoudOS

uint32_t get_usec(void);                            // Retourne le nombre de nanosecondse qui se sont écoulé dans cette seconde
uint32_t get_pit_count(void);                       // Retourne la valeur actuelle du compteur du PIT 

void TimerTick(struct cpu_state *regs);
