#pragma once
#include <types.h>
#include <core/dev/char/tty.h>

#define ecma48_state uint32_t
#define ecma48_param uint32_t
#define ecma48_index uint32_t

#define ECMA48_STANDBY			0			// En attente d'une suite de contrôle ECMA 48
#define ECMA48_ESCAPE			1			// Le caractère escape(\033) a été reçu
#define ECMA48_ESCAPE_CTRL		2			// La suite de caractère "\033[" a été recu
#define ECMA48_GET_PARAMETERS	3			// Les paramètres sont en cours de récupération
#define ECMA48_PROCESSING		4			// On traite la suite de contrôle

#define ECMA48_MAXPARAM			16

hret_t console_write(const struct tty_device *tty, char c);
void csi_m(void);