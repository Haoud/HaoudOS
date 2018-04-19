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
#include <core/mm/heap.h>
#include <lib/semaphore.h>
#include <core/process/sleep.h>

hret_t release_semaphore(struct semaphore *sem)
{
    hret_t ret;
    if(!sem)
        return -ERR_BAD_ARG;

    ret = release_wait_queue(sem->wait);

    if(ret != RET_OK)
        return ret;

    kfree(sem);
    return RET_OK;
}

hret_t init_semaphore(struct semaphore **sem, int initial_value)
{
    (*sem) = kmalloc(sizeof(struct semaphore));

    if(!(*sem))
        return -ERR_NO_MEM;

    (*sem)->value = initial_value;
    init_wait_queue(&(*sem)->wait);

    return RET_OK;
}

hret_t up_semaphore(struct semaphore *sem)
{
    uint32_t flags;
    hret_t ret = RET_OK;

    lock_int(flags);
        sem->value++;
        wake_up(sem->wait);
    unlock_int(flags);
    return ret;
}

hret_t down_semaphore(struct semaphore *sem)
{
    uint32_t flags;
    hret_t ret = RET_OK;
    
    lock_int(flags);
        while(sem->value <= 0)              // Tant que le sémaphore n'est pas libre
            sleep_on(sem->wait);            // On s'endors d'un sommeil ininterruptible
                    
        sem->value--;
    unlock_int(flags);
    return ret;
}

hret_t trydown_semaphore(struct semaphore *sem)
{
    uint32_t flags;
    hret_t ret;
    
    lock_int(flags);
        if(sem->value >= 1)         // Pouvons nous obtenir directement le sémaphore ?
        {
            sem->value--;           // Oui, on décrémente le nombre de place libre
            ret = RET_OK;           // Tout s'est bien passé
        }
        else
        {
            ret = -ERR_BUSY;        // Non, on retourne un code d'erreur
        }   

    unlock_int(flags);
    return ret;
}
