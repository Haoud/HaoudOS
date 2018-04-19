#include <maths.h>
#include <i386/i8254.h>
#include <core/time/time.h>
#include <core/time/cmos.h>

static struct date startup_date;		// Heure de d�marrage
static time_t unix_startup_time;		// Heure Unix du d�marrage d'HaoudOS

/*
 * Permet de d�terminer statiquement (si l'ann�e n'est pas bisextille) combien de secondes se sont
 * �coul�es du d�but de l'ann�e jusqu'au mois souhait� (0 = janvier, 11 = d�cembre)
 */
static int sec_month[12] = {
	0,
	SECONDS_IN_DAY *(31),
	SECONDS_IN_DAY *(31 + 28),
	SECONDS_IN_DAY *(31 + 28 + 31),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30 + 31),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30 + 31 + 30),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30 + 31 + 30 + 31),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31),
	SECONDS_IN_DAY *(31 + 28 + 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30)
};

void init_time(void)
{
	mkdate(&startup_date);
	unix_startup_time = kernel_mktime(&startup_date);
}

void mkdate(struct date *dt)
{
	dt->century = bcd_to_decimal(cmos_read(CMOS_REG_CENTURY));
	dt->years = bcd_to_decimal(cmos_read(CMOS_REG_DATE_YEAR));
	dt->month = bcd_to_decimal(cmos_read(CMOS_REG_DATE_MONTH)) - 1;
	dt->month_day = bcd_to_decimal(cmos_read(CMOS_REG_DATE_DAY));
	dt->week_day = bcd_to_decimal(cmos_read(CMOS_REG_WEEK_DAY));
	dt->hours = bcd_to_decimal(cmos_read(CMOS_REG_HOURS));
	dt->minutes = bcd_to_decimal(cmos_read(CMOS_REG_MINUTES));
	dt->seconds = bcd_to_decimal(cmos_read(CMOS_REG_SECONDS));

	if (dt->week_day)
		dt->week_day--;
	else
		dt->week_day = 6;

	/*
	 * Ce code tr�s simple permet de d�terminer si nous sommes en heure d'�t� ou nom, malheureusement, ce code
	 * n'est pas pr�cis et donc peut avancer ou retarder de 1 � 6 jours en fonction de l'ann�e...
	 */
	
	if (dt->month > 3 && dt->month < 10)
		dt->summer_hour = TRUE;
	else
		dt->summer_hour = FALSE;
}

time_t get_current_unix_time(void)
{
	return unix_startup_time + get_startup_sec();
}

time_t kernel_mktime(struct date *dt)
{
	time_t years = 0;
	time_t unix_time = 0;
	time_t years_after_70 = 0;				// Nombre d'ann�e � partir de 1970
	//time_t sec_in_this_years = 0;

	years_after_70 = (dt->century * 100 + dt->years);
	years_after_70 = years_after_70 - 1970;

	unix_time = dt->seconds;
	unix_time += dt->minutes * SECONDS_IN_MINUTE;
	unix_time += dt->hours * SECONDS_IN_HOUR;
	unix_time += dt->month_day * SECONDS_IN_DAY;
	unix_time += sec_month[dt->month];
	unix_time += SECONDS_IN_DAY * ((years_after_70) / 4);			// Prend en compte le jour supl�mentaire des ann�es bissextilles
	unix_time += SECOND_IN_YEAR * years_after_70;

	if (dt->month > 1 && (years % 4))
		unix_time += SECONDS_IN_DAY;

	/* Prend en compte l'heure d'�t� / d'hiver car l'heure UNIX utilise normalement l'heure UTC (l'heure universelle) alors
	 * que nous utilisons l'heure locale fran�aise qui implique un d�calage de 2 ou 1 heure si nous sommes en heure d'�t� ou
	 * non.
	 * Si le syst�me reste allum� alors que le changement d'heure est en cours, il ne sera pas pris en compte tant que le syst�me
	 * restera allum�.
	 */

	if (dt->summer_hour == TRUE)
		unix_time -= SECONDS_IN_HOUR * 2;
	else
		unix_time -= SECONDS_IN_HOUR;

	return unix_time;
}