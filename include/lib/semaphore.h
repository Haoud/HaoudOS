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
#include <core/process/wait.h>

#define MUTEX           1

struct semaphore
{
    int value;                  // Nombre de place restante de disponible
    struct wait_queue *wait;    // Processus en attente du sémaphore
};

hret_t release_semaphore(struct semaphore *sem);
hret_t init_semaphore(struct semaphore **sem, int initial_value);

hret_t up_semaphore(struct semaphore *sem);
hret_t down_semaphore(struct semaphore *sem);
hret_t trydown_semaphore(struct semaphore *sem);

