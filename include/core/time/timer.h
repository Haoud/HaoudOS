#pragma once
#include <types.h>
#include <lib/list.h>

struct timer
{
	time_t sec_expire;							// Expiration en seconde
	time_t tick_expire;							// Expiration en tick qui complète le variables sec_expire

	uint32_t data;								// Données à passer à la fonction lorsque le timer expire
	void (*expire_function)(uint32_t data);		// Pointeur vers la fonction à appeler lorsque le timer expire

	struct timer *prev;
	struct timer *next;
};

extern struct timer *timer_list;				// Liste chainé trié en fonction de l'expiration du timer

void update_timers(void);
void init_timer_driver(void);
void add_timer(struct timer *to_add);
void del_timer(struct timer *to_del);
void init_timer(struct timer *to_init);
void update_one_timer(struct timer *to_update);
void set_timer(struct timer *to_set, uint32_t ms_expire_time);
