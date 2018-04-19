#include <types.h>
#include <assert.h>
#include <i386/cpu.h>
#include <i386/idt.h>
#include <lib/stdio.h>
#include <lib/ctype.h>
#include <lib/stdlib.h>
#include <lib/string.h>
#include <i386/i8259.h>
#include <i386/ioports.h>
#include <core/process/sleep.h>
#include <driver/bochs/bochs.h>
#include <driver/keyboard/kbdmapfr.h>
#include <driver/keyboard/keyboard.h>

static bool_t _alt;
static bool_t _ctrl;
static bool_t _shift;
static bool_t _altgr;

static bool_t _alt_locked;
static bool_t _ctrl_locked;
static bool_t _shift_locked;

static bool_t _e0;

static char **old_kbdmap;			// Pour sauvegarde current_kbdmap
static char **current_kbdmap;		// Pointeur sur le tableau de mappage des touches actuel (fr, en, de, shift, maj...)

static struct tty_device *current_tty;		// Terminal qui recevera les frappes du clavier

void Keyboard_IRQ(void);					// Handler de l'interruption du clavier
extern void asm_Keyboard_IRQ(void);

/*
 * Cette fonction e charge d'initialise le driver du clavier, c'est à dire toutes les
 * variables concernant le driver et le controleur du clavier (IRQ, ports I/O...)
 */
void SetupKeyboard(void)
{
	_e0 = FALSE;

	_alt = FALSE;
	_ctrl = FALSE;
	_shift = FALSE;

	_alt_locked = FALSE;
	_ctrl_locked = FALSE;
	_shift_locked = FALSE;

	current_tty = NULL;
	current_kbdmap = kbdmap_fr;

	BochsPutc('\n');

	MakeIdtDescriptor(KBD_IDT_VECTOR, (uint32_t)asm_Keyboard_IRQ, 0x08, IDT_INT_GATE32 | IDT_HANDLER_PRESENT);
	i8259_EnableIrq(KBD_IRQ_NR);					// Autorise l'IRQ du clavier

	BochsPrintf("[INFO] PS/2 Keyboard initialized to IRQ %u (IDT 0x%x)\n", KBD_IRQ_NR, KBD_IDT_VECTOR);
}


/*
 * C'est cette fonction qui est appelée lorsque l'utilisateur interagit avec le
 * clavier (touche presée, relachée...)
 * Elle sauvegarde le contexte d'éxécution au début de l'interruption puis le
 * restore à la fin de celle-ci, afin de pouvoir continuer à éxécuter la tâche
 * qui a été suspendu par l'interruption.
 */
void Keyboard_IRQ(void)
{
	Keyboard_bh();
	i8259_SendEOI(KBD_IRQ_NR);
}

/*
 * Cette fonction permet de déterminer quelle touche a été préssée (ou relaché) et de
 * actualiser le tampon des touches ASCII pressés
 */
void Keyboard_bh(void)
{
	int scancode, makecode, breakcode;
	char * keycode;

	KeyboardWaitOutBufferFull();			// Attend que le buffer de sortie du clavier soit plein (ou time out)
	scancode = inb(KBD_ENC_DATA_PORT);		// Récupération du scancode

	AssertFatal(scancode != 0x0);			// Normalement le clavier n'envoie pas de scancode nul

	if (scancode == 0xE0)
	{
		_e0 = TRUE;
		old_kbdmap = current_kbdmap;			// Sauvegarde la table de mappage actuelle
		current_kbdmap = e0_kbdmap_fr;			// Table spécial pour les touches préfixées par 0xe0
	}
	else if (IsBreakCode(scancode))
	{
		// Break code = Touche relachée
		breakcode = BreakCodeToScanCode(scancode);
		keycode = current_kbdmap[breakcode];

		// S'il faut changer de table de mapping (Table de mapping avec MAJ, CTRL...)
		if (!strcmp(keycode, KEY_RSHIFT) || !strcmp(keycode, KEY_LSHIFT))
		{
			_shift = FALSE;
			current_kbdmap = kbdmap_fr;
		}
		else if (!strcmp(keycode, KEY_ALT))
		{
			_alt = FALSE;
			current_kbdmap = kbdmap_fr;
		}
		else if (!strcmp(keycode, KEY_ALTGR))
		{
			_altgr = FALSE;
			current_kbdmap = kbdmap_fr;
		}
		else if (!strcmp(keycode, KEY_CTRL))
		{
			_ctrl = FALSE;
			current_kbdmap = kbdmap_fr;
		}

	}
	else
	{
		// Make code = Touche pressée
		makecode = MakeCodeToScanCode(scancode);
		keycode = current_kbdmap[makecode];

		// S'il faut changer de table de mapping (Table de mapping avec MAJ, CTRL...)
		if (!strcmp(keycode,KEY_RSHIFT) || !strcmp(keycode, KEY_LSHIFT))
		{
			_shift = TRUE;
			current_kbdmap = shift_kbdmap_fr;
		}
		else if (!strcmp(keycode, KEY_ALT))
		{
			_alt = TRUE;
			current_kbdmap = alt_gr_kbdmap_fr;
		}
		else if (!strcmp(keycode, KEY_ALTGR))
		{
			_altgr = TRUE;
			current_kbdmap = alt_gr_kbdmap_fr;
		}
		else if (!strcmp(keycode, KEY_CTRL))
		{
			_ctrl = TRUE;
			current_kbdmap = ctrl_kbdmap_fr;
		}
		else
		{
			if (current_tty != NULL)
				tty_add_chars(current_tty, keycode);
		}
	}

	
	if (scancode != 0xE0 && _e0 == TRUE)
	{
		_e0 = FALSE;
		current_kbdmap = old_kbdmap;			// Restoration de l'ancienne table de mappage
	}
}

void kbd_set_current_tty(struct tty_device *tty)
{
	current_tty = tty;
}

/*
 * Cette fonction permet d'attendre que le buffer de sortie du contrôleur 
 * du clavier soit plein. Si le temps d'attente est trop long alors on 
 * quitte la fonction en retournant 0
 */
int KeyboardWaitOutBufferFull(void)
{
	int time_out = KBD_CTL_TIME_OUT;
	uint8_t port = 0;

	do
	{
		port = inb(KBD_CTL_STAT_PORT);
		time_out--;
	} while (!(port & KBD_CTL_STAT_OUT_BUF_FULL) && time_out);

	return time_out;
}

int KeyboardEncSendCommand(uint8_t cmd)
{
	int time_out = KBD_CTL_TIME_OUT;
	while ( !(inb(KBD_CTL_STAT_PORT) & KBD_CTL_STAT_IN_BUF_FULL) && time_out)
		time_out--;

	/* Envoi de la commande à l'encoder du clavier */
	outb(KBD_ENC_CMD, cmd);

	return time_out;
}

int KeyboardCtlSendCommand(uint8_t cmd)
{
	int time_out = KBD_CTL_TIME_OUT;
	while (!(inb(KBD_CTL_STAT_PORT) & KBD_CTL_STAT_IN_BUF_FULL) && time_out)
		time_out--;

	/* Envoi de la commande à l'encoder du clavier */
	outb(KBD_CTL_CMD_PORT, cmd);

	return time_out;
}