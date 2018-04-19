#include <lib/list.h>
#include <lib/stdio.h>
#include <lib/ctype.h>
#include <core/fs/super.h>

static struct super_block *mount_point[MAX_MOUNT];

/*
 * Cette fonction convetit une lettre de lecteur (comme C:) en un 
 * index de tableau utilisable directement dans mount_point
 */
int letter_to_index(const char drive_letter)
{
	if (!is_letter(drive_letter))
		panic("Lettre de lecteur invalide !!");

	if (is_lower(drive_letter))
		return drive_letter - 'a';
	else
		return drive_letter - 'A';
}

/*
 * Cette fonction tr�s simple mais essentielle permet de retourner le super block
 * associ� � une lettre de montage comme C:
 */
struct super_block *get_super(char drive)
{
	return mount_point[letter_to_index(drive)];
}

/*
 * Cette fonction permet d'ajouter un super block op�rationnel dans la liste des super
 * block et le rend accessible en lui assignant une lettre de lecteur. Si celle ci est
 * utilis�e, alors le noyau s'arr�te
 */
void add_super(struct super_block *super, char drive_letter)
{
	if (mount_point[letter_to_index(drive_letter)] != NULL)
		panic("Tentative d'utilisation d'une lettre de lecteur occup�e");

	if (list_empty(super->fs->super_list))
		list_singleton(super->fs->super_list, super);
	else
		list_add_after(super->fs->super_list, super);					// Ajoute le super block � la liste des supers blo�cks qui utilisent ce fs

	mount_point[letter_to_index(drive_letter)] = super;				// Rend le super block accessible � l'utilisateur
}

/*
 * Cette fonction permet de supprimer un super block op�rationnel dans la liste des super
 * block et le rend inacessible � l'utilisateur. Si aucun super block ne correpond � la
 * lettre de montage, le noyau s'arr�te
 */
void remove_super(char drive_letter)
{
	struct super_block *to_remove = get_super(drive_letter);		// Obtient le super block � rend inacessible

	if (to_remove == NULL)
		panic("Tentative de suppresion d'une lettre de montage libre");

	list_delete(to_remove->fs->super_list, to_remove);				// Retire le super block � la liste des supers blocks qui utilisent ce fs
	mount_point[letter_to_index(drive_letter)] = NULL;				// Rend le super block inexistant pour l'utilisateur
}

/*
 * Cette fonction initialise la liste de montage avec les valeurs par d�fauts (NULL)
 */
void init_mount_list(void)
{
	for (int i = 0; i < MAX_MOUNT; i++)
		mount_point[i] = NULL;

	debugk("[VFS] Mount point list initialised\n");
}