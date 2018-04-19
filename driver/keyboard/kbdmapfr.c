#include <driver/keyboard/keyboard.h>

char *kbdmap_fr[] = {
	KEY_UNKNOW,				// Keycode 0
	KEY_ESCAPE,				// Keycode 1 (touche echap)
	KEY_EPERLUETTE,			// Keycode 2 (&)
	KEY_UNKNOW,				// Keycode 3 (é, mais pas encore implémenté)
	KEY_DOUBLEQUOTE,		// Keycode 4 (")
	KEY_QUOTE,				// Keycode 5 (')
	KEY_LEFTPARENTHESIS,	// Keycode 6 ('(')
	KEY_TIRET,				// Keycode 7 (-)
	KEY_UNKNOW,				// Keycode 8 (è, mais pas encore implémenté)
	KEY_UNDERSCORE,			// Keycode 9 (_)
	KEY_UNKNOW,				// Keycode 10 (ç, mais pas encore implémenté)
	KEY_UNKNOW,				// Keycode 11 (à, mais pas encore implémenté)
	KEY_RIGHTPARENTHESIS,	// Keycode 12 (')')
	KEY_EGAL,				// Keycode 13 (=)
	KEY_BACKSPACE,			// Keycode 14 (Touche retour arrière)
	KEY_TABULATION,			// Keycode 15 (Tabulation)
	KEY_A,					// Keycode 16 (a)
	KEY_Z,					// Keycode 17 (z)
	KEY_E,					// Keycode 18 (e)
	KEY_R,					// Keycode 19 (r)
	KEY_T,					// Keycode 20 (t)
	KEY_Y,					// Keycode 21 (y)
	KEY_U,					// Keycode 22 (u)
	KEY_I,					// Keycode 23 (i)
	KEY_O,					// Keycode 24 (o)
	KEY_P,					// Keycode 25 (p)
	KEY_CARRY,				// Keycode 26 (^)
	KEY_DOLLAR,				// Keycode 27 ($)
	KEY_RETURN,				// Keycode 28 (Touche entrée)
	KEY_CTRL,				// Keycode 29 (Touche CTRL de droite ou de gauche)
	KEY_Q,					// Keycode 30 (q)
	KEY_S,					// Keycode 31 (s)
	KEY_D,					// Keycode 32 (d)
	KEY_F,					// Keycode 33 (f)
	KEY_G,					// Keycode 34 (g)
	KEY_H,					// Keycode 35 (h)
	KEY_J,					// Keycode 36 (j)
	KEY_K,					// Keycode 37 (k)
	KEY_L,					// Keycode 38 (l)
	KEY_M,					// Keycode 39 (m)
	KEY_UNKNOW,				// Keycode 40 (ù, mais non implémenté)
	KEY_UNKNOW,				// Keycode 41 (², mais non implémenté)
	KEY_RSHIFT,				// Keycode 42 (Touche SHIFT de gauche)
	KEY_ASTERIX,			// Keycode 43 (*)
	KEY_W,					// Keycode 44 (w)
	KEY_X,					// Keycode 45 (x)
	KEY_C,					// Keycode 46 (c)
	KEY_V,					// Keycode 47 (v)
	KEY_B,					// Keycode 48 (b)
	KEY_N,					// Keycode 49 (n)
	KEY_COMMA,				// Keycode 50 (,)
	KEY_SEMICOLON,			// Keycode 51 (;)
	KEY_COLON,				// Keycode 52 (:)
	KEY_EXCLAMATION,		// Keycode 53 (!)
	KEY_LSHIFT,				// Keycode 54 (Touche SHIFT de droite)
	KEY_UNKNOW,				// Keycode 55 (Touche Impr screen, non implémenté)
	KEY_ALT,				// Keycode 56 (Touche ALT gauche)
	KEY_ESPACE,				// Keycode 57 (Barre espace, vous savez, celle qui prend la moitié du clavier :) )
	KEY_UNKNOW,				// Keycode 58 (Touche Verr Maj, non implementée)
	KEY_F1,					// Keycode 59 (Touche F1)
	KEY_F2,					// Keycode 60 (Touche F2)
	KEY_F3,					// Keycode 61 (Touche F3)
	KEY_F4,					// Keycode 62 (Touche F4)
	KEY_F5,					// Keycode 63 (Touche F5)
	KEY_F6,					// Keycode 64 (Touche F6)
	KEY_F7,					// Keycode 65 (Touche F7)
	KEY_F8,					// Keycode 66 (Touche F8)
	KEY_F9,					// Keycode 67 (Touche F9)
	KEY_F10,				// Keycode 68 (Touche F10)
	KEY_VERR_NUM,			// Keycode 69 (Touche Verr num du pavé numérique)
	KEY_UNKNOW,				// Keycode 70 (Touche Win. Lock, nom implementée)
	KEY_7,					// Keycode 71 (7 du pavé numérique)
	KEY_8,					// Keycode 72 (8 du pavé numérique)
	KEY_9,					// Keycode 73 (9 du pavé numérique)
	KEY_MINUS,				// Keycode 74 (touche moins du pavé numérique)
	KEY_4,					// Keycode 75 (4 du pavé numérique)
	KEY_5,					// Keycode 76 (5 du pavé numérique)
	KEY_6,					// Keycode 77 (6 du pavé numérique)
	KEY_PLUS,				// Keycode 78 (+ du système numérique)
	KEY_1,					// Keycode 79 (1 du pavé numérique)
	KEY_2,					// Keycode 80 (2 du pavé numérique)
	KEY_3,					// Keycode 81 (3 du pavé numérique)
	KEY_0,					// Keycode 82 (0 du pavé numérique)
	KEY_DOT,				// Keycode 83 (. du pavé numérique)
	KEY_UNKNOW,				// Keycode 84
	KEY_UNKNOW,				// Keycode 85
	KEY_LESS				// Keycode 86 (<)
};

char *shift_kbdmap_fr[] = {
	KEY_UNKNOW,				// Keycode 0
	KEY_ESCAPE,				// Keycode 1 (touche echap)
	KEY_1,					// Keycode 2 (1)
	KEY_2,					// Keycode 3 (2)
	KEY_3,					// Keycode 4 (3)
	KEY_4,					// Keycode 5 (4)
	KEY_5,					// Keycode 6 (5)
	KEY_6,					// Keycode 7 (6)
	KEY_7,					// Keycode 8 (7)
	KEY_8,					// Keycode 9 (8)
	KEY_9,					// Keycode 10 (9)
	KEY_0,					// Keycode 11 (0)
	KEY_UNKNOW,				// Keycode 12 (°, mais non implémentée)
	KEY_PLUS,				// Keycode 13 (+)
	KEY_BACKSPACE,			// Keycode 14 (Touche retour arrière)
	KEY_TABULATION,			// Keycode 15 (Tabulation)
	KEY_MAJ_A,				// Keycode 16 (A)
	KEY_MAJ_Z,				// Keycode 17 (Z)
	KEY_MAJ_E,				// Keycode 18 (E)
	KEY_MAJ_R,				// Keycode 19 (R)
	KEY_MAJ_T,				// Keycode 20 (T)
	KEY_MAJ_Y,				// Keycode 21 (Y)
	KEY_MAJ_U,				// Keycode 22 (U)
	KEY_MAJ_I,				// Keycode 23 (I)
	KEY_MAJ_O,				// Keycode 24 (O)
	KEY_MAJ_P,				// Keycode 25 (P)
	KEY_UNKNOW,				// Keycode 26 (¨, mais non implementée)
	KEY_UNKNOW,				// Keycode 27 (£, mais non implémentée)
	KEY_RETURN,				// Keycode 28 (Touche entrée)
	KEY_CTRL,				// Keycode 29 (Touche CTRL de droite ou de gauche)
	KEY_MAJ_Q,				// Keycode 30 (Q)
	KEY_MAJ_S,				// Keycode 31 (S)
	KEY_MAJ_D,				// Keycode 32 (D)
	KEY_MAJ_F,				// Keycode 33 (F)
	KEY_MAJ_G,				// Keycode 34 (G)
	KEY_MAJ_H,				// Keycode 35 (H)
	KEY_MAJ_J,				// Keycode 36 (J)
	KEY_MAJ_K,				// Keycode 37 (K)
	KEY_MAJ_L,				// Keycode 38 (L)
	KEY_MAJ_M,				// Keycode 39 (M)
	KEY_PERCENT,			// Keycode 40 (%)
	KEY_UNKNOW,				// Keycode 41 (², mais non implémenté)
	KEY_RSHIFT,				// Keycode 42 (Touche SHIFT de gauche)
	KEY_UNKNOW,				// Keycode 43 (µ, mais non implémenté)
	KEY_MAJ_W,				// Keycode 44 (W)
	KEY_MAJ_X,				// Keycode 45 (X)
	KEY_MAJ_C,				// Keycode 46 (C)
	KEY_MAJ_V,				// Keycode 47 (V)
	KEY_MAJ_B,				// Keycode 48 (B)
	KEY_MAJ_N,				// Keycode 49 (N)
	KEY_INTERROGATION,		// Keycode 50 (?)
	KEY_DOT,				// Keycode 51 (.)
	KEY_SLASH,				// Keycode 52 (/)
	KEY_UNKNOW,				// Keycode 53 (§, mais non implémenté)
	KEY_LSHIFT,				// Keycode 54 (Touche SHIFT de droite)
	KEY_UNKNOW,				// Keycode 55 (Touche Impr screen, non implémenté)
	KEY_ALT,				// Keycode 56 (Touche ALT de droite ou gauche)
	KEY_ESPACE,				// Keycode 57 (Barre espace, vous savez, celle qui prend la moitié du clavier :) )
	KEY_UNKNOW,				// Keycode 58 (Touche Verr Maj, non implementée)
	KEY_F1,					// Keycode 59 (Touche F1)
	KEY_F2,					// Keycode 60 (Touche F2)
	KEY_F3,					// Keycode 61 (Touche F3)
	KEY_F4,					// Keycode 62 (Touche F4)
	KEY_F5,					// Keycode 63 (Touche F5)
	KEY_F6,					// Keycode 64 (Touche F6)
	KEY_F7,					// Keycode 65 (Touche F7)
	KEY_F8,					// Keycode 66 (Touche F8)
	KEY_F9,					// Keycode 67 (Touche F9)
	KEY_F10,				// Keycode 68 (Touche F10)
	KEY_VERR_NUM,			// Keycode 69 (Touche Verr num du pavé numérique)
	KEY_UNKNOW,				// Keycode 70 (Touche Win. Lock, nom implementée)
	KEY_7,					// Keycode 71 (7 du pavé numérique)
	KEY_8,					// Keycode 72 (8 du pavé numérique)
	KEY_9,					// Keycode 73 (9 du pavé numérique)
	KEY_MINUS,				// Keycode 74 (touche moins du pavé numérique)
	KEY_4,					// Keycode 75 (4 du pavé numérique)
	KEY_5,					// Keycode 76 (5 du pavé numérique)
	KEY_6,					// Keycode 77 (6 du pavé numérique)
	KEY_PLUS,				// Keycode 78 (+ du système numérique)
	KEY_1,					// Keycode 79 (1 du pavé numérique)
	KEY_2,					// Keycode 80 (2 du pavé numérique)
	KEY_3,					// Keycode 81 (3 du pavé numérique)
	KEY_0,					// Keycode 82 (0 du pavé numérique)
	KEY_DOT,				// Keycode 83 (. du pavé numérique) 
	KEY_UNKNOW,				// Keycode 84
	KEY_UNKNOW,				// Keycode 85
	KEY_GREATER				// Keycode 86 (>)
};

char *alt_kbdmap_fr[] = { 
	KEY_UNKNOW,				// Keycode 0
	KEY_UNKNOW,				// Keycode 1 (touche echap)
	KEY_UNKNOW,				// Keycode 2
	KEY_UNKNOW,				// Keycode 3
	KEY_UNKNOW,				// Keycode 4
	KEY_UNKNOW,				// Keycode 5
	KEY_UNKNOW,				// Keycode 6
	KEY_UNKNOW,				// Keycode 7
	KEY_UNKNOW,				// Keycode 8
	KEY_UNKNOW,				// Keycode 9
	KEY_UNKNOW,				// Keycode 10
	KEY_UNKNOW				// Keycode 11
	KEY_UNKNOW,				// Keycode 12
	KEY_UNKNOW,				// Keycode 13
	KEY_RETURN,				// Keycode 14 (Touche retour arrière)
	KEY_TABULATION,			// Keycode 15 (Tabulation)
	KEY_UNKNOW,				// Keycode 16
	KEY_UNKNOW,				// Keycode 17
	KEY_UNKNOW,				// Keycode 18
	KEY_UNKNOW,				// Keycode 19
	KEY_UNKNOW,				// Keycode 20
	KEY_UNKNOW,				// Keycode 21
	KEY_UNKNOW,				// Keycode 22
	KEY_UNKNOW,				// Keycode 23
	KEY_UNKNOW,				// Keycode 24
	KEY_UNKNOW,				// Keycode 25
	KEY_UNKNOW,				// Keycode 26
	KEY_UNKNOW,				// Keycode 27
	KEY_UNKNOW,				// Keycode 28
	KEY_CTRL,				// Keycode 29 (Touche CTRL de droite ou de gauche)
	KEY_UNKNOW,				// Keycode 30
	KEY_UNKNOW,				// Keycode 31
	KEY_UNKNOW,				// Keycode 32
	KEY_UNKNOW,				// Keycode 33
	KEY_UNKNOW,				// Keycode 34
	KEY_UNKNOW,				// Keycode 35
	KEY_UNKNOW,				// Keycode 36
	KEY_UNKNOW,				// Keycode 37
	KEY_UNKNOW,				// Keycode 38
	KEY_UNKNOW,				// Keycode 39
	KEY_UNKNOW,				// Keycode 40
	KEY_UNKNOW,				// Keycode 41
	KEY_UNKNOW,				// Keycode 42 (Touche SHIFT de gauche)
	KEY_UNKNOW,				// Keycode 43
	KEY_UNKNOW,				// Keycode 44
	KEY_UNKNOW,				// Keycode 45
	KEY_UNKNOW,				// Keycode 46
	KEY_UNKNOW,				// Keycode 47
	KEY_UNKNOW,				// Keycode 48
	KEY_UNKNOW,				// Keycode 49
	KEY_UNKNOW,				// Keycode 50
	KEY_UNKNOW,				// Keycode 51
	KEY_UNKNOW,				// Keycode 52
	KEY_UNKNOW,				// Keycode 53
	KEY_LSHIFT,				// Keycode 54 (Touche SHIFT de droite)
	KEY_UNKNOW,				// Keycode 55
	KEY_ALT,				// Keycode 56 (Touche ALT de droite ou gauche)
	KEY_ESPACE,				// Keycode 57 (Barre espace, vous savez, celle qui prend la moitié du clavier :) )
	KEY_UNKNOW,				// Keycode 58
	KEY_F1,					// Keycode 59 (Touche F1)
	KEY_F2,					// Keycode 60 (Touche F2)
	KEY_F3,					// Keycode 61 (Touche F3)
	KEY_F4,					// Keycode 62 (Touche F4)
	KEY_F5,					// Keycode 63 (Touche F5)
	KEY_F6,					// Keycode 64 (Touche F6)
	KEY_F7,					// Keycode 65 (Touche F7)
	KEY_F8,					// Keycode 66 (Touche F8)
	KEY_F9,					// Keycode 67 (Touche F9)
	KEY_F10,				// Keycode 68 (Touche F10)
	KEY_VERR_NUM,			// Keycode 69 (Touche Verr num du pavé numérique)
	KEY_UNKNOW,				// Keycode 70
	KEY_UNKNOW,				// Keycode 71 
	KEY_UNKNOW,				// Keycode 72
	KEY_UNKNOW,				// Keycode 73
	KEY_UNKNOW,				// Keycode 74 
	KEY_UNKNOW,				// Keycode 75 
	KEY_UNKNOW,				// Keycode 76
	KEY_UNKNOW,				// Keycode 77
	KEY_UNKNOW,				// Keycode 78 
	KEY_UNKNOW,				// Keycode 79
	KEY_UNKNOW,				// Keycode 80 
	KEY_UNKNOW,				// Keycode 81 
	KEY_UNKNOW,				// Keycode 82 
	KEY_UNKNOW,				// Keycode 83
};

char *alt_gr_kbdmap_fr[] = {
	KEY_UNKNOW,				// Keycode 0
	KEY_ESCAPE,				// Keycode 1 (touche echap)
	KEY_UNKNOW,				// Keycode 2
	KEY_UNKNOW,				// Keycode 3
	KEY_HASHTAG,			// Keycode 4 (#)
	KEY_LEFTCURL,			// Keycode 5 ({)
	KEY_LEFTBRACKET,		// Keycode 6 ([)
	KEY_BARRE,				// Keycode 7 (|)
	KEY_UNKNOW,				// Keycode 8
	KEY_BACKSLASH,			// Keycode 9 (\)
	KEY_UNKNOW,				// Keycode 10
	KEY_AT,					// Keycode 11 (@)
	KEY_RIGHTBRACKET,		// Keycode 12 (])
	KEY_RIGHTCURL,			// Keycode 13 (+)
	KEY_BACKSPACE,			// Keycode 14 (Touche retour arrière)
	KEY_TABULATION,			// Keycode 15 (Tabulation)
	KEY_MAJ_A,				// Keycode 16 (A)
	KEY_MAJ_Z,				// Keycode 17 (Z)
	KEY_MAJ_E,				// Keycode 18 (E)
	KEY_MAJ_R,				// Keycode 19 (R)
	KEY_MAJ_T,				// Keycode 20 (T)
	KEY_MAJ_Y,				// Keycode 21 (Y)
	KEY_MAJ_U,				// Keycode 22 (U)
	KEY_MAJ_I,				// Keycode 23 (I)
	KEY_MAJ_O,				// Keycode 24 (O)
	KEY_MAJ_P,				// Keycode 25 (P)
	KEY_UNKNOW,				// Keycode 26
	KEY_UNKNOW,				// Keycode 27
	KEY_RETURN,				// Keycode 28 (Touche entrée)
	KEY_CTRL,				// Keycode 29 (Touche CTRL de droite ou de gauche)
	KEY_MAJ_Q,				// Keycode 30 (Q)
	KEY_MAJ_S,				// Keycode 31 (S)
	KEY_MAJ_D,				// Keycode 32 (D)
	KEY_MAJ_F,				// Keycode 33 (F)
	KEY_MAJ_G,				// Keycode 34 (G)
	KEY_MAJ_H,				// Keycode 35 (H)
	KEY_MAJ_J,				// Keycode 36 (J)
	KEY_MAJ_K,				// Keycode 37 (K)
	KEY_MAJ_L,				// Keycode 38 (L)
	KEY_MAJ_M,				// Keycode 39 (M)
	KEY_PERCENT,			// Keycode 40 (%)
	KEY_UNKNOW,				// Keycode 41
	KEY_RSHIFT,				// Keycode 42 (Touche SHIFT de gauche)
	KEY_UNKNOW,				// Keycode 43
	KEY_MAJ_W,				// Keycode 44 (W)
	KEY_MAJ_X,				// Keycode 45 (X)
	KEY_MAJ_C,				// Keycode 46 (C)
	KEY_MAJ_V,				// Keycode 47 (V)
	KEY_MAJ_B,				// Keycode 48 (B)
	KEY_MAJ_N,				// Keycode 49 (N)
	KEY_INTERROGATION,		// Keycode 50 (?)
	KEY_DOT,				// Keycode 51 (.)
	KEY_SLASH,				// Keycode 52 (/)
	KEY_UNKNOW,				// Keycode 53
	KEY_LSHIFT,				// Keycode 54 (Touche SHIFT de droite)
	KEY_UNKNOW,				// Keycode 55
	KEY_ALT,				// Keycode 56 (Touche ALT de droite ou gauche)
	KEY_ESPACE,				// Keycode 57 (Barre espace, vous savez, celle qui prend la moitié du clavier :) )
	KEY_UNKNOW,				// Keycode 58
	KEY_F1,					// Keycode 59 (Touche F1)
	KEY_F2,					// Keycode 60 (Touche F2)
	KEY_F3,					// Keycode 61 (Touche F3)
	KEY_F4,					// Keycode 62 (Touche F4)
	KEY_F5,					// Keycode 63 (Touche F5)
	KEY_F6,					// Keycode 64 (Touche F6)
	KEY_F7,					// Keycode 65 (Touche F7)
	KEY_F8,					// Keycode 66 (Touche F8)
	KEY_F9,					// Keycode 67 (Touche F9)
	KEY_F10,				// Keycode 68 (Touche F10)
	KEY_VERR_NUM,			// Keycode 69 (Touche Verr num du pavé numérique)
	KEY_UNKNOW,				// Keycode 70
	KEY_7,					// Keycode 71 (7 du pavé numérique)
	KEY_8,					// Keycode 72 (8 du pavé numérique)
	KEY_9,					// Keycode 73 (9 du pavé numérique)
	KEY_MINUS,				// Keycode 74 (touche moins du pavé numérique)
	KEY_4,					// Keycode 75 (4 du pavé numérique)
	KEY_5,					// Keycode 76 (5 du pavé numérique)
	KEY_6,					// Keycode 77 (6 du pavé numérique)
	KEY_PLUS,				// Keycode 78 (+ du système numérique)
	KEY_1,					// Keycode 79 (1 du pavé numérique)
	KEY_2,					// Keycode 80 (2 du pavé numérique)
	KEY_3,					// Keycode 81 (3 du pavé numérique)
	KEY_0,					// Keycode 82 (0 du pavé numérique)
	KEY_DOT,				// Keycode 83 (. du pavé numérique) 
};

char *ctrl_kbdmap_fr[] = {
	KEY_UNKNOW,				// Keycode 0
	KEY_UNKNOW,				// Keycode 1 (touche echap)
	KEY_UNKNOW,				// Keycode 2
	KEY_UNKNOW,				// Keycode 3
	KEY_UNKNOW,				// Keycode 4
	KEY_UNKNOW,				// Keycode 5
	KEY_UNKNOW,				// Keycode 6
	KEY_UNKNOW,				// Keycode 7
	KEY_UNKNOW,				// Keycode 8
	KEY_UNKNOW,				// Keycode 9
	KEY_UNKNOW,				// Keycode 10
	KEY_UNKNOW,				// Keycode 11
	KEY_UNKNOW,				// Keycode 12
	KEY_UNKNOW,				// Keycode 13
	KEY_RETURN,				// Keycode 14 (Touche retour arrière)
	KEY_TABULATION,			// Keycode 15 (Tabulation)
	"\x01",					// Keycode 16 (A)
	"\x1A",					// Keycode 17 (Z)
	"\x05",					// Keycode 18 (E)
	"\x12",					// Keycode 19 (R)
	"\x14",					// Keycode 20 (T)
	"\x19",					// Keycode 21 (Y)
	"\x15",					// Keycode 22 (U)
	"\x09",					// Keycode 23 (I)
	"\x0F",					// Keycode 24 (O)
	"\x10",					// Keycode 25 (P)
	KEY_UNKNOW,				// Keycode 26
	KEY_UNKNOW,				// Keycode 27
	KEY_UNKNOW,				// Keycode 28
	KEY_CTRL,				// Keycode 29 (Touche CTRL de droite ou de gauche)
	"\x11",					// Keycode 30 (Q)
	"\x13",					// Keycode 31 (S)
	"\x04",					// Keycode 32 (D)
	"\x06",					// Keycode 33 (F)
	"\x07",					// Keycode 34 (G)
	"\x08",					// Keycode 35 (H)
	"\x0A",					// Keycode 36 (J)
	"\x0B",					// Keycode 37 (K)
	"\x0C",					// Keycode 38 (L)
	"\x0D",					// Keycode 39 (M)
	KEY_UNKNOW,				// Keycode 40
	KEY_UNKNOW,				// Keycode 41
	KEY_LSHIFT,				// Keycode 42 (Touche SHIFT de gauche)
	KEY_UNKNOW,				// Keycode 43
	"\x17",					// Keycode 44 (W)
	"\x18",					// Keycode 45 (X)
	"\x03",					// Keycode 46 (C)
	"\x16",					// Keycode 47 (V)
	"\x02",					// Keycode 48 (B)
	"\x0E",					// Keycode 49 (N)
	KEY_UNKNOW,				// Keycode 50
	KEY_UNKNOW,				// Keycode 51
	KEY_UNKNOW,				// Keycode 52
	KEY_UNKNOW,				// Keycode 53
	KEY_LSHIFT,				// Keycode 54 (Touche SHIFT de droite)
	KEY_UNKNOW,				// Keycode 55
	KEY_ALTGR,				// Keycode 56 (Touche ALT de droite ou gauche)
	KEY_ESPACE,				// Keycode 57 (Barre espace, vous savez, celle qui prend la moitié du clavier :) )
	KEY_UNKNOW,				// Keycode 58
	KEY_F1,					// Keycode 59 (Touche F1)
	KEY_F2,					// Keycode 60 (Touche F2)
	KEY_F3,					// Keycode 61 (Touche F3)
	KEY_F4,					// Keycode 62 (Touche F4)
	KEY_F5,					// Keycode 63 (Touche F5)
	KEY_F6,					// Keycode 64 (Touche F6)
	KEY_F7,					// Keycode 65 (Touche F7)
	KEY_F8,					// Keycode 66 (Touche F8)
	KEY_F9,					// Keycode 67 (Touche F9)
	KEY_F10,				// Keycode 68 (Touche F10)
	KEY_VERR_NUM,			// Keycode 69 (Touche Verr num du pavé numérique)
	KEY_UNKNOW,				// Keycode 70
	KEY_UNKNOW,				// Keycode 71 
	KEY_UNKNOW,				// Keycode 72
	KEY_UNKNOW,				// Keycode 73
	KEY_UNKNOW,				// Keycode 74 
	KEY_UNKNOW,				// Keycode 75 
	KEY_UNKNOW,				// Keycode 76
	KEY_UNKNOW,				// Keycode 77
	KEY_UNKNOW,				// Keycode 78 
	KEY_UNKNOW,				// Keycode 79
	KEY_UNKNOW,				// Keycode 80 
	KEY_UNKNOW,				// Keycode 81 
	KEY_UNKNOW,				// Keycode 82 
	KEY_UNKNOW,				// Keycode 83
};

char *e0_kbdmap_fr[] = {
	KEY_UNKNOW,				// Keycode 0
	KEY_UNKNOW,				// Keycode 1
	KEY_UNKNOW,				// Keycode 2
	KEY_UNKNOW,				// Keycode 3
	KEY_UNKNOW,				// Keycode 4
	KEY_UNKNOW,				// Keycode 5
	KEY_UNKNOW,				// Keycode 6
	KEY_UNKNOW,				// Keycode 7
	KEY_UNKNOW,				// Keycode 8
	KEY_UNKNOW,				// Keycode 9
	KEY_UNKNOW,				// Keycode 10
	KEY_UNKNOW,				// Keycode 11
	KEY_UNKNOW,				// Keycode 12
	KEY_UNKNOW,				// Keycode 13
	KEY_UNKNOW,				// Keycode 14
	KEY_UNKNOW,				// Keycode 15
	KEY_UNKNOW,				// Keycode 16
	KEY_UNKNOW,				// Keycode 17
	KEY_UNKNOW,				// Keycode 18
	KEY_UNKNOW,				// Keycode 19
	KEY_UNKNOW,				// Keycode 20
	KEY_UNKNOW,				// Keycode 21
	KEY_UNKNOW,				// Keycode 22
	KEY_UNKNOW,				// Keycode 23
	KEY_UNKNOW,				// Keycode 24
	KEY_UNKNOW,				// Keycode 25
	KEY_UNKNOW,				// Keycode 26
	KEY_UNKNOW,				// Keycode 27
	KEY_UNKNOW,				// Keycode 28
	KEY_UNKNOW,				// Keycode 29
	KEY_UNKNOW,				// Keycode 30
	KEY_UNKNOW,				// Keycode 31
	KEY_UNKNOW,				// Keycode 32
	KEY_UNKNOW,				// Keycode 33
	KEY_UNKNOW,				// Keycode 34
	KEY_UNKNOW,				// Keycode 35
	KEY_UNKNOW,				// Keycode 36
	KEY_UNKNOW,				// Keycode 37
	KEY_UNKNOW,				// Keycode 38
	KEY_UNKNOW,				// Keycode 39
	KEY_UNKNOW,				// Keycode 40
	KEY_UNKNOW,				// Keycode 41
	KEY_UNKNOW,				// Keycode 42 
	KEY_UNKNOW,				// Keycode 43
	KEY_UNKNOW,				// Keycode 44
	KEY_UNKNOW,				// Keycode 45
	KEY_UNKNOW,				// Keycode 46
	KEY_UNKNOW,				// Keycode 47
	KEY_UNKNOW,				// Keycode 48
	KEY_UNKNOW,				// Keycode 49
	KEY_UNKNOW,				// Keycode 50
	KEY_UNKNOW,				// Keycode 51
	KEY_UNKNOW,				// Keycode 52
	KEY_UNKNOW,				// Keycode 53
	KEY_UNKNOW,				// Keycode 54 
	KEY_UNKNOW,				// Keycode 55
	KEY_UNKNOW,				// Keycode 56 
	KEY_UNKNOW,				// Keycode 57
	KEY_UNKNOW,				// Keycode 58
	KEY_UNKNOW,				// Keycode 59
	KEY_UNKNOW,				// Keycode 60
	KEY_UNKNOW,				// Keycode 61
	KEY_UNKNOW,				// Keycode 62
	KEY_UNKNOW,				// Keycode 63
	KEY_UNKNOW,				// Keycode 64
	KEY_UNKNOW,				// Keycode 65
	KEY_UNKNOW,				// Keycode 66
	KEY_UNKNOW,				// Keycode 67
	KEY_UNKNOW,				// Keycode 68
	KEY_UNKNOW,				// Keycode 69 
	KEY_UNKNOW,				// Keycode 70
	KEY_UNKNOW,				// Keycode 71 
	KEY_ARROW_UP,			// Keycode 72
	KEY_PAGE_UP,			// Keycode 73
	KEY_UNKNOW,				// Keycode 74 
	KEY_ARROW_LEFT,			// Keycode 75 
	KEY_UNKNOW,				// Keycode 76
	KEY_ARROW_RIGHT,		// Keycode 77
	KEY_UNKNOW,				// Keycode 78 
	KEY_UNKNOW,				// Keycode 79
	KEY_ARROW_DOWN,			// Keycode 80 
	KEY_PAGE_DOWN,			// Keycode 81 
	KEY_UNKNOW,				// Keycode 82 
	KEY_UNKNOW				// Keycode 83
};

