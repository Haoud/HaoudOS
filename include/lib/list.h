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
#pragma GCC system_header		// Supprime des avertissement inutiles
#include <types.h>

/*
 * Fonction permettant d'implementer une liste doublement chainé circulaire en C. Ces macros
 * ne vérifient pas les conditions anormales, c'est à la fonction appelante de le faire, en
 * particulier vérifier si on ne pâsse pas NULL en argument de ces macros.
 * Ces macros fonctionnent avec des structures qui possèdent des champs se nommant "prev" et
 * "next"
 *
 * Note: DC --> Doublement Chainé
 */


/* Initialise la liste DC circulaire comme étant une liste vide */
#define list_init(list)							\
	((list) = NULL)

/* Initialise la liste avec un seul élément: la liste */
#define list_singleton(list, item)						\
	list_singleton_named(list, item, prev, next)


/* Initialise la liste avec un seul élément: la liste */
#define list_singleton_named(list, item, prev,next)		\
	({(item)->next = (item);							\
	(item)->prev = (item);								\
	(list) = (item);})							



/* Cherche à déterminer si la liste est vide (aucun élément) */
#define list_empty(list)						\
	((list) == NULL)

/* Retourne le début de la liste doublement chainée circulaire */
#define list_get_head(list)						\
	(list)

#define list_get_head_named(list, prev, next)	\
	(list)


/* Retourne la fin de la liste doublement chainée circulaire */
#define list_get_tail(list)						\
	((list)->prev)

/* Ajoute un élément item après after */
#define list_add_after_named(after, item, prev, next)({		\
	(after)->next->prev = (item);							\
	(item)->next = (after)->next;							\
	(after)->next = (item);									\
	(item)->prev = (after);})					

#define list_add_after(after, item)							\
	list_add_after_named(after, item, prev, next)			


/* Ajoute un élément item avant before */
#define list_add_before_named(before, item, prev, next)({		\
	(before)->prev->next = (item);								\
	(item)->prev = (before)->prev;								\
	(before)->prev = (item);									\
	(item)->next = (before);})					
	
#define list_add_before(before, item)							\
	list_add_before_named(before, item, prev, next)		

/* Parcourt toute la liste du début à la fin */
#define list_foreach(list, current, nb_element)					\
	for((current) = (list), nb_element = 0; (current) != (list) || !nb_element; (current) = (current)->next, nb_element++)
		
#define list_foreach_named(list, current, nb_element, prev, next)					\
	for((current) = (list), nb_element = 0; (current) != (list) || !nb_element; (current) = (current)->next, nb_element++)

#define list_foreach_safe(list, current, pnext, nb_element)					\
		for ((current) = (list), (pnext) = (list)->next, nb_element = 0;		\
			((current) != (list) || !nb_element) && (list) != NULL;			\
			(current) = (pnext), (pnext) = (pnext)->next, nb_element++)

#define list_foreach_safe_named(list, current, pnext, nb_element, prev, next)					\
	for ((current) = (list), (pnext) = (list)->next, nb_element = 0;		\
		 ((current) != (list) || !nb_element) && (list) != NULL;			\
		 (current) = (pnext), (pnext) = (pnext)->next, nb_element++)


#define list_delete_named(list,item, prev, next)				\
	if ( ((item)->next == (item)) && ((item)->prev == (item)) )	\
    {															\
		(item)->next = (item)->prev = (list) = NULL;			\
	}															\
	else														\
	{															\
		(item)->prev->next = (item)->next;						\
		(item)->next->prev = (item)->prev;						\
		if ((item) == (list))									\
			(list) = (item)->next;								\
		(item)->prev = NULL;									\
		(item)->next = NULL;									\
	}

#define list_delete(list,item)									\
	list_delete_named(list, item, prev, next)