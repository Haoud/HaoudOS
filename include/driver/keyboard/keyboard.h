#pragma once
#include <types.h>
#include <core/dev/char/tty.h>

/*
 * Aide à la compréhension du code  des macros et des fonction:
 * ENC = Encoder
 * CTL = Controleur
 * STAT = Statut
 * CMD = Commande
 * BUF = Buffer
 * ERR = Erreur
 */

//=============================================================================
// ==========================| Encodeur du clavier |===========================
//=============================================================================

#define KBD_ENC_DATA_PORT		0x60		// Port de données de l'encoder
#define KBD_ENC_CMD				0x60		// Port de commande de l'encoder

#define KBD_ENC_CMD_SET_LEDS	0xED
#define KBD_ENC_CMD_ECHO		0xEE

// Completer la liste des commandes de l'encodeur  ...

#define KBD_ENC_CMD_RESEND		0xFE
#define KBD_ENC_CMD_RESET		0xFF


//=============================================================================
// =========================| Controleur du clavier |==========================
//=============================================================================

#define KBD_CTL_CMD_PORT		0x64		// Port de commande du controleur
#define KBD_CTL_STAT_PORT		0x64		// Port de statut du controleur

#define KBD_CTL_STAT_OUT_BUF_FULL		0x01
#define KBD_CTL_STAT_IN_BUF_FULL		0x02
#define KBD_CTL_STAT_SYS_FLAG			0x04
#define KBD_CTL_STAT_COMMAND			0x08
#define KBD_CTL_STAT_LOCKED				0x10
#define KBD_CTL_STAT_SECOND_BUF_FULL	0x20
#define KBD_CTL_STAT_TIME_OUT			0x40
#define KBD_CTL_STAT_PARITY_ERR			0x80


#define KBD_CTL_CMD_READ				0x20
#define KBD_CTL_CMD_WRITE				0x60
#define KBD_CTL_CMD_SELF_TEST			0xAA
#define KBD_CTL_CMD_INTERFACE_TEST		0xAB
#define KBD_CTL_CMD_ENABLE_KBD			0xAE
#define KBD_CTL_CMD_DISABLE_KBD			0xAD
// Completer la liste des commandes du controleur...


//=============================================================================
//=============================================================================
//=============================================================================

// Completer la liste des erreurs possible pour le clavier ...

	
#define KBD_IDT_VECTOR		0x21		// Le vecteur d'interruption réservé au clavier
#define KBD_IRQ_NR			1			// Le clavier possède l'IRQ 1

#define KBD_CTL_TIME_OUT	2000		// Nombre d'intinération avant time out

/*
 * Certaines touches n'ont pas de valeurs ASCII car elle ne réprésente rien
 * à l'écran mais ont une valeur pour les reconnaitre entre elle (par exemple 
 * la touche SHIFT  la touche INSERT  la touche RETOUR ...): leurs valeurs doit
 * être superieurs à 256 car les valeurs en dessous sont réservés au caractères
 * ASCII étendus affichable (e  é  ÿ  # ...)
 * Le touche qui possède des valeurs ASCII peuvent être affiché à l'écran
 */
#define KEY_A  "a" 
#define KEY_B  "b" 
#define KEY_C  "c" 
#define KEY_D  "d" 
#define KEY_E  "e"
#define KEY_F  "f" 
#define KEY_G  "g" 
#define KEY_H  "h" 
#define KEY_I  "i" 
#define KEY_J  "j" 
#define KEY_K  "k" 
#define KEY_L  "l" 
#define KEY_M  "m" 
#define KEY_N  "n" 
#define KEY_O  "o" 
#define KEY_P  "p" 
#define KEY_Q  "q" 
#define KEY_R  "r" 
#define KEY_S  "s" 
#define KEY_T  "t" 
#define KEY_U  "u" 
#define KEY_V  "v" 
#define KEY_W  "w" 
#define KEY_X  "x" 
#define KEY_Y  "y" 
#define KEY_Z  "z" 

#define KEY_MAJ_A  "A" 
#define KEY_MAJ_B  "B" 
#define KEY_MAJ_C  "C" 
#define KEY_MAJ_D  "D" 
#define KEY_MAJ_E  "E" 
#define KEY_MAJ_F  "F" 
#define KEY_MAJ_G  "G" 
#define KEY_MAJ_H  "H" 
#define KEY_MAJ_I  "I" 
#define KEY_MAJ_J  "J" 
#define KEY_MAJ_K  "K" 
#define KEY_MAJ_L  "L" 
#define KEY_MAJ_M  "M" 
#define KEY_MAJ_N  "N" 
#define KEY_MAJ_O  "O" 
#define KEY_MAJ_P  "P" 
#define KEY_MAJ_Q  "Q" 
#define KEY_MAJ_R  "R" 
#define KEY_MAJ_S  "S" 
#define KEY_MAJ_T  "T" 
#define KEY_MAJ_U  "U" 
#define KEY_MAJ_V  "V" 
#define KEY_MAJ_W  "W" 
#define KEY_MAJ_X  "X" 
#define KEY_MAJ_Y  "Y" 
#define KEY_MAJ_Z  "Z" 

#define KEY_0  "0" 
#define KEY_1  "1" 
#define KEY_2  "2" 
#define KEY_3  "3" 
#define KEY_4  "4" 
#define KEY_5  "5" 
#define KEY_6  "6" 
#define KEY_7  "7" 
#define KEY_8  "8" 
#define KEY_9  "9" 

#define KEY_MINUS  "-" 
#define KEY_PLUS  "+" 
#define KEY_EGAL  "=" 
#define KEY_SLASH  "/" 
#define KEY_ASTERIX  "*" 
#define KEY_PERCENT  "%" 
#define KEY_CARRY  "^" 

#define KEY_DOLLAR  "$" 
#define KEY_LIVRE  "£" 
#define KEY_EURO  "€" 

#define KEY_BACKSLASH  "\\" 
#define KEY_DOT  "." 
#define KEY_EXCLAMATION  "!" 
#define KEY_INTERROGATION  "?" 
#define KEY_COMMA  "," 
#define KEY_COLON  ":" 
#define KEY_SEMICOLON  ";" 

#define KEY_AT  "@" 
#define KEY_HASHTAG  "#" 
#define KEY_DOUBLEQUOTE  "\"" 
#define KEY_QUOTE  "'" 

#define KEY_EPERLUETTE  "&" 
#define KEY_UNDERSCORE  "_" 
#define KEY_TIRET  "-" 
#define KEY_BARRE  "|" 
#define KEY_LEFTPARENTHESIS  "(" 
#define KEY_RIGHTPARENTHESIS  ")" 
#define KEY_LEFTBRACKET  "[" 
#define KEY_RIGHTBRACKET  "]" 
#define KEY_LEFTCURL  "{" 
#define KEY_RIGHTCURL  "}" 
#define KEY_LESS  "<" 
#define KEY_GREATER  ">" 

#define KEY_RETURN  "\n" 
#define KEY_BACKSPACE  "\b" 
#define KEY_TABULATION  "\t" 
#define KEY_ESPACE  " " 

#define KEY_ESCAPE "\e001"		
#define KEY_LSHIFT "\e002"		
#define KEY_RSHIFT "\e003"
#define KEY_CTRL   "\e004"
#define KEY_ALT    "\e005"

#define KEY_F1 "\e006"
#define KEY_F2 "\e007"
#define KEY_F3 "\e008"
#define KEY_F4 "\e009"
#define KEY_F5 "\e010"
#define KEY_F6 "\e011"
#define KEY_F7 "\e012"
#define KEY_F8 "\e013"
#define KEY_F9 "\e014"
#define KEY_F10 "\e015"
#define KEY_F11 "\e016"
#define KEY_F12 "\e017"

#define KEY_ARROW_UP    "\033[A"
#define KEY_ARROW_DOWN  "\033[B"
#define KEY_ARROW_RIGHT "\033[C"
#define KEY_ARROW_LEFT  "\033[D"

#define KEY_PAGE_UP     "\033[5~"
#define KEY_PAGE_DOWN   "\033[6~"

#define KEY_VERR_NUM "\e018"
#define KEY_ALTGR "\e019"
#define KEY_UNKNOW ""

char getch(void);												// Fonction bloquante

char user_getch(void);											// Idem à getch mais suspend le processus
void Keyboard_bh(void);											// Fonction qui gère la partie basse du clavier
void SetupKeyboard(void);
int KeyboardWaitOutBufferFull(void);
int KeyboardEncSendCommand(uint8_t cmd);
int KeyboardCtlSendCommand(uint8_t cmd);
void kbd_set_current_tty(struct tty_device *tty);

#define IsBreakCode(x)	(x & 0x80)
#define IsMakeCode(x)	(!IsBreakCode(x))

#define MakeCodeToScanCode(x)	(x)								// Le make code est identique au scan code
#define BreakCodeToScanCode(x)	(x - 0x80)						// Il suffit de soustraire 128 au break code pour obtenir le scan code