#include <lib/stdio.h>
#include <i386/i8254.h>
#include <core/time/timer.h>

struct timer *timer_list;

/*
 * Cette fonction simple initialise le driver des timers d'HaoudOS
 */
void init_timer_driver(void)
{
	list_init(timer_list);
}

/*
 * Cette fonction permet d'initialiser une structure timer passé en argument;
 * seul les variables prev et next sont modifié
 */
void init_timer(struct timer *to_init)
{
	to_init->prev = NULL;
	to_init->next = NULL;
}

void debug_chain_list(void)
{
	struct timer *current_timer = NULL;
	int nb_timer;

	debugk("***** START OF TIMER LIST *****\n");
	list_foreach(timer_list, current_timer, nb_timer)
	{
		debugk("n° %i: Expire at %u sec %u tick\n", nb_timer, current_timer->sec_expire, current_timer->tick_expire);
	}
	debugk("****** END Of TIMER LIST ******\n");

}


/*
 * Cette fonction permet d'ajouter un timer (struct timer) complet dans la liste
 * des timers.
 * Si le timer est déjà expiré, alors ce dernier n'est pas ajouté à la liste mais
 * la fonction de rappel est bien appelé.
 */
void add_timer(struct timer *to_add)
{
	struct timer *add_after = NULL;
	int nb_timer;

	time_t current_sec = get_startup_sec();
	time_t current_tick = get_tick_in_this_sec();

	if (to_add != NULL)
	{
		/* Si le timer est déjà expiré */
		if (to_add->sec_expire < current_sec || 
		   (to_add->sec_expire == current_sec && to_add->tick_expire <= current_tick))
		{
			to_add->expire_function(to_add->data);				// Appele la fonction de rappel
			return;												
		}

		if (list_empty(timer_list))
		{
			// Si la liste des timers est vide, on l'initialise
			list_singleton(timer_list, to_add);
		}
		else if (to_add->sec_expire < timer_list->sec_expire ||
				(to_add->sec_expire == timer_list->sec_expire && to_add->tick_expire < timer_list->sec_expire))
		{
			list_add_before(timer_list, to_add);
			timer_list = to_add;
		}
		else
		{
			// Sinon on ajoute la structure timer au bon endroit, c'est
			// à dire trié par ordre croissant (expire_sec et expire_tick)

			list_foreach(timer_list, add_after, nb_timer)
			{
				if (to_add->sec_expire < add_after->sec_expire)
				{
					list_add_before(add_after, to_add);
					return;
				}

				if (to_add->sec_expire == add_after->sec_expire &&
					to_add->tick_expire <= add_after->tick_expire)
				{
					list_add_before(add_after, to_add);
					return;
				}
			}

			list_add_before(timer_list, to_add);
		}
	}
}

/*
 * Cette fonction permet de désenregistrer un timer de la liste des timers
 * actifs. Si le timer n'existe pas dans la liste, il ne se passe rien
 *
 * Lorsque le timer expire, il est automatiquement supprimé de cette liste, 
 * mais c'est un bonne habitude de "confirmer" la suppression du timer en
 * appelant une 2ème fois cette fonction, manuellement.
 */
void del_timer(struct timer *to_del)
{
	if (list_empty(timer_list))
		return;

	struct timer *looked_timer;
	int nb_timer;

	list_foreach(timer_list, looked_timer, nb_timer)
	{
		if (looked_timer == to_del)
		{
			list_delete(timer_list, to_del);
			return;
		}
	}
}

/*
 * Cette fonction permet de vérifier si un timer vient d'expirer, si c'est le
 * cas, alors on éxécute la fonction du timer avant de le supprimer de la liste
 * des timers actif
 */
void update_one_timer(struct timer *to_update)
{
	time_t current_sec = get_startup_sec();
	time_t current_tick = get_tick_in_this_sec();

	if (to_update->sec_expire < current_sec)
		goto execute_function;

	if (to_update->sec_expire == current_sec &&
		to_update->tick_expire <= current_tick)
		goto execute_function;

	return;

execute_function:
	to_update->expire_function(to_update->data);			// Execute la fonction d'expiration du timer
	del_timer(to_update);									// Supprime le timer
	return;
}

/*
 * Cette fonctionn permet d'actualiser tout les objets de la liste des
 * timers actifs qui ont besoin de l'être (qui viennent juste d'expirer)
 * Cette fonction actualise seulement les timers qui viennent d'expirer afin
 * d'éviter de devoir parcourir toute la liste chainée, ce qui induirait une
 * charge au processeur plus importante
 */
void update_timers(void)
{
	if (list_empty(timer_list))
		return;

	int nb_timers;
	struct timer *next_timer;
	struct timer *looked_timer;

	time_t current_sec = get_startup_sec();
	time_t current_tick = get_tick_in_this_sec();

	list_foreach_safe(timer_list, looked_timer, next_timer, nb_timers)
	{
		if (looked_timer->sec_expire > current_sec)
			break;

		if (looked_timer->sec_expire == current_sec &&
			looked_timer->tick_expire > current_tick)
			break;

		update_one_timer(looked_timer); 
	}
}

void set_timer(struct timer *to_set, uint32_t ms_expire_time)
{
	to_set->sec_expire = get_startup_sec() + ms_expire_time / 1000;
	to_set->tick_expire = get_tick_in_this_sec() + (ms_expire_time % 1000) / (1000 / PIT_8254_DEFAULT_FRENQUENCY);

	if (to_set->tick_expire >= PIT_8254_DEFAULT_FRENQUENCY)
	{
		to_set->sec_expire++;
		to_set->tick_expire -= PIT_8254_DEFAULT_FRENQUENCY;
	}
}