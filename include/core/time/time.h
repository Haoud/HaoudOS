#pragma once
#include <types.h>

#define SECONDS_IN_MINUTE	60									// Nombre de secondes dans une minute
#define SECONDS_IN_HOUR		(SECONDS_IN_MINUTE * 60)			// Nombre de secondes dans une heure
#define SECONDS_IN_DAY		(SECONDS_IN_HOUR * 24)				// Nombre de secondes dans un jour
#define SECOND_IN_YEAR		(SECONDS_IN_DAY * 365)				// Nombre de secondes dans une année NON BISSEXTILLE

#define CURRENT_UNIX_TIME	get_current_unix_time();

struct date
{
	time_t century;			// Siècle (1900, 2000...) mais n'indique que les 2ers chiffre (19, 20 ...)
	time_t years;			// Nombre d'année dans le siècle (de 0 à 99)
	time_t month;			// Mois de l'année (de 0 à 11: 0 = Janvier, 1 = Février ... 11 = Décembre)
	time_t month_day;		// Jour dans le mois (de 0 à 31)
	time_t week_day;		// Jours de la semaine (de 0 à 6: 0 = Lundi, 6 = Dimanche)
	time_t hours;			// Heures
	time_t minutes;			// Minutes de l'heure
	time_t seconds;			// Secondes de la minute
	time_t summer_hour;		// Heure d'été ? (-1 = Aucune information, 0 = heure d'hiver, 1 = heure d'été)
};

void init_time(void);
void mkdate(struct date *dt);
time_t get_current_unix_time(void);
time_t kernel_mktime(struct date *dt);
