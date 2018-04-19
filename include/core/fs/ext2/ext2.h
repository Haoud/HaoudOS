/*
 * This file was created on Wed Apr 11 2018
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
#include <types.h>
#include <const.h>
#include <core/fs/vfs.h>
#include <core/fs/file.h>
#include <core/fs/super.h>
#include <core/fs/inode.h>
#include <core/fs/dirent.h>
#include <core/process/process.h>

#define EXT2_MAGIC                  0xEF53

#define EXT2_BLOCK_SIZE             1024               
#define EXT2_NAME_LENGHT             255
#define EXT2_INODE_ROOT             2

#define EXT2_STATE_CLEAN            1
#define EXT2_STATE_ERROR            2

#define EXT2_ERROR_IGNORE           1
#define EXT2_ERROR_REMOUNT_RO       2
#define EXT2_ERROR_PANIC            3

#define EXT2_CREATOR_HAOUDOS        1234567890
#define EXT2_CREATOR_LINUX          0
#define EXT2_CREATOR_HURD           1
#define EXT2_CREATOR_MASIX          2
#define EXT2_CREATOR_FREEBSD        3
#define EXT2_CREATOR_LITES          4

#define EXT2_OPTIONAL_FEATURE_PREALLOC              0x0001
#define EXT2_OPTIONAL_FEATURE_AFS                   0x0002
#define EXT2_OPTIONAL_FEATURE_JOURNAL               0x0004
#define EXT2_OPTIONAL_FEATURE_EXTENDED_INODE        0x0008    
#define EXT2_OPTIONAL_FEATURE_CAN_REZISE            0x0010
#define EXT2_OPTIONAL_FEATURE_DIR_HASH              0x0020

#define EXT2_REQUIED_FEATURE_COMPRESSION           0x0001
#define EXT2_REQUIED_FEATURE_DIR_TYPE              0x0002
#define EXT2_REQUIED_FEATURE_NEED_REPLAY           0x0004
#define EXT2_REQUIED_FEATURE_JOURNAL_DEVICE        0x0008    

#define EXT2_RO_FEATURE_SPARSE                     0x0001
#define EXT2_RO_FEATURE_64BIT_SIZE                 0x0002
#define EXT2_RO_FEATURE_BINARY_TREE                0x0004

#define EXT2_INODE_MAX_BLOCKS       15

#define EXT2_INODE_UNKNOW           0x0000
#define EXT2_INODE_FIFO             0x1000
#define EXT2_INODE_CHAR             0x2000
#define EXT2_INODE_DIR              0x4000
#define EXT2_INODE_BLOCK            0x6000
#define EXT2_INODE_REG              0x8000
#define EXT2_INODE_SYMBOLIC_LINK    0xA000
#define EXT2_INODE_SOCKET           0xC000*

#define EXT2_INODE_ISUID    0x0800  // Bit "setuid"
#define EXT2_INODE_ISGID    0x0400  // Bit "setgid"
#define EXT2_INODE_ISVTX    0x0200  // Bit sticky (obselète)
#define EXT2_INODE_IRWXU    0x01C0  // Masque des droits d'accès de l'utilisateur
#define EXT2_INODE_IRUSR    0x0100  // Lecture
#define EXT2_INODE_IWUSR    0x0080  // Ecriture
#define EXT2_INODE_IXUSR    0x0040  // Execution
#define EXT2_INODE_IRWXG    0x0038  // Masque des droits d'accès du groupe
#define EXT2_INODE_IRGRP    0x0020  // Lecture 
#define EXT2_INODE_IWGRP    0x0010  // Ecriture
#define EXT2_INODE_IXGRP    0x0008  // Execution
#define EXT2_INODE_IRWXO    0x0007  // Masque des droits d'accès des autres utilisateurs
#define EXT2_INODE_IROTH    0x0004  // Lecture
#define EXT2_INODE_IWOTH    0x0002  // Ecriture
#define EXT2_INODE_IXOTH    0x0001  // Execution

#define EXT2_DIRENT_UNKNOW           0
#define EXT2_DIRENT_REG              1
#define EXT2_DIRENT_DIR              2
#define EXT2_DIRENT_CHAR             3
#define EXT2_DIRENT_BLOCK            4
#define EXT2_DIRENT_PIPE             5
#define EXT2_DIRENT_SOCKET           6
#define EXT2_DIRENT_SYMBOLIC_LINK    7

#define EXT2_NEED_INDIRECT1(s, idx)        (idx > 11 && idx <= 11 + (s->block_size / 4))
#define EXT2_NEED_INDIRECT2(s, idx)        (idx > 11 + (s->block_size / 4) && idx <= 11 + (s->block_size / 4) * (s->block_size / 4) + (s->block_size / 4))
#define EXT2_NEED_INDIRECT3(s, idx)        (idx > 11 + (s->block_size / 4) * (s->block_size / 4) + (s->block_size / 4))

#define EXT2_DIRENT_BASE_SIZE       8          // Taille d'une entrée de répertoire sans le nom

struct ext2_disk_super
{
    uint32_t inodes_count;          // Nombre d'inodes dans le système de fichier
    uint32_t block_count;           // Nombre de bloc dans le système de fichier
    uint32_t root_block_count;      // Nombre de block réservés au super utilisateur
    uint32_t free_inodes_count;     // Nombre d'inodes libres dans le système de fichier
    uint32_t free_block_count;      // Nombre de bloc libres dans le système de fichier
    uint32_t first_data_block;      // Numéro de block contenant le super block
    uint32_t log2_block_size;       // Taille des block en log2 (0 = 1024, 1 = 2046...)
    uint32_t log2_fragment_size;    // Taille de fragment en log2  
    uint32_t blocks_per_group;      // Nombre de bloc par groupe
    uint32_t fragments_per_group;   // Nombre de fragments par groupe
    uint32_t inodes_per_group;      // Nombre d'inodes par groupe
    uint32_t last_mount_time;       // Dernière heure de montage
    uint32_t last_write_time;       // Dernière heure d'écriture
    uint16_t mount_count;           // Compteur de montage
    uint16_t max_mount_count;       // Nombre de montage max avant le contrôle du fs
    uint16_t magic;                 // Nombre magique
    uint16_t state;                 // Etat du système de fichier
    uint16_t errors;                // Comportements lors de la détéction d'erreur
    uint16_t minor_revision;        // Nombre mineur de révision
    uint32_t last_check_time;       // Heure du dernier contrôle
    uint32_t check_interval;        // Intervalle entre 2 vérification
    uint32_t creator_os;            // OS créateur du système de fichier
    uint32_t major_revision;        // Nombre majeur de révision
    uint16_t default_resuid;        // UID par défaut pour les block réservés
    uint16_t default_regid;         // GID par défaut pour les block réservés
    uint32_t first_inode;           // Numéro du premier inode non réservé
    uint16_t inode_size;            // Taille des inodes sur le disque
    uint16_t block_group_nr;        // Numéro du groupe de bloc de ce super block
    uint32_t feature_optional;      // Bitmap des fonctionnalités optionnelles supportés
    uint32_t feature_required;      // Bitmap des fonctionnalités requises supportés
    uint32_t feature_ro_compat;     // Bitmap des fonctionnalités supportés en lecture seure
    uint8_t uuid[16];               // Identificateur du système de fichier sur 128 bits
    char volume_name[16];           // Nom du volume
    char last_mounted[64];          // Emplacement du dernier point de montage. Pour HaoudOS, c'est seulement une lettre (C, D...)
    uint32_t algo_bitmap;           // Utilisé pour le compression ???
    uint8_t prealloc_block;         // Nombre de block à préallouer pour les fichiers
    uint8_t prealloc_dir_block;     // Nombre de block à préallouer pour les fichiers pour les répertoires
}_packed;

struct ext2_memory_super
{
    struct ext2_disk_super super;  // Structure du super block sur le disque

    /* Variables uniquement en mémoire */
    bool_t is_dirty;                // Determine s'il faut réécrire le super block sur le disque
    uint32_t block_size;            // Taille des blocs en octets
    uint32_t nb_groups;             // Nombre de groupes sur le FS
}_packed;

struct ext2_group_descriptor
{
    uint32_t block_bitmap;          // Numéro de block du bitmap des blocks
    uint32_t inode_bitmap;          // Numéro de block du bitmap des inodes
    uint32_t inode_table;           // Numéro du bloc du premier bloc des la table des inodes

    uint16_t free_blocks_count;     // Nombre de block libre dans le groupe
    uint16_t free_inode_count;      // Nombre d'inodes libres dans le groupe
    uint16_t used_dirs_count;       // Nombre de répertoire dans le groupe
    uint16_t padding;               // Alignement
    uint32_t reserved[3];           // Réservés pour que la structure fasse 12 octets
}_packed;

struct ext2_inode
{
    uint16_t mode;                  // TYpe de fichier et droits d'accès
    uint16_t uid;                   // Identificateur du propriétaire
    uint32_t size;                  // Taille du fichier en octet
    uint32_t last_acces_time;       // Heure du dernière accès
    uint32_t creation_time;         // Heure de création du fichier
    uint32_t last_modif_time;       // Heure de la dernière modification
    uint32_t delete_time;           // Heure de suppresion du fichier
    uint16_t gid;                   // Identificateur de groupe
    uint16_t link_count;            // Compteur de liens sur le disque
    uint32_t nb_blocks;             // Nombre de blocs de données du fichier
    uint32_t flags;                 // Options du fichier
    uint32_t osd1;                  // Spécifique au système de fichier
    uint32_t block[EXT2_INODE_MAX_BLOCKS];      // Blocs de données
    uint32_t version;               // Version du fichier (Pour les fs sans état, comme NFS)
    uint32_t file_acl;              // Liste de contrôle d'accès au fichier
    uint32_t dir_acl;               // Liste de contrôle d'accès au dossier
    uint32_t fragment_addr;         // Adresse du fragment
    uint8_t osd2[12];               // Spécifique au système de fichier
}_packed;

struct ext2_dirent
{
    uint32_t inode;                 // Numéro d'inode de l'entrée
    uint16_t entry_lenght;          // Taille de la structure ext2_dirent (La taille du nom est variable)
    uint8_t name_lenght;            // Taille du nom
    uint8_t file_type;              // Type de l'entrée de répertoire
    char name[EXT2_NAME_LENGHT];
}_packed;

void RegisterExt2(void);            // Enregistre le driver ext2 auprès du VFS

/* Fonctions requises par le VFS */
hret_t ext2_open_file(struct inode *this_inode, struct process *owner, flags_t open_flags, struct file **result);
hret_t ext2_link(struct inode *this_inode, const struct process *actor, const char *name, struct inode *inode_to_link);
hret_t ext2_duplicate_open_file(struct file *this_file, struct process *for_process, struct file **result);
hret_t ext2_allocate_inode(struct super_block *this_block, enum inode_type type, struct inode **result);
hret_t ext2_fetch_inode(struct super_block *this_super, uint64_t inode_id, struct inode **result);
hret_t ext2_mount(struct file_system *this_fs, dev_t device, struct super_block **result_super);
hret_t ext2_lookup(struct inode *this_inode, const char *name, uint64_t *result_inode_id);
hret_t ext2_unlink(struct inode *this_inode, struct process *actor, const char *name);
hret_t ext2_seek(struct file *this_file, seek_t whence, off_t offset, off_t *result);
hret_t ext2_close_file(struct inode _unused *this_inode, struct file *to_close);
hret_t ext2_umount(struct file_system *this_fs, struct super_block *to_umount);
hret_t ext2_close_file(struct inode *this_inode, struct file *to_close);
hret_t ext2_write(struct file *this_file, char *src_buf, size_t *len);

hret_t ext2_read(struct file *this_file, char *dst_buf, size_t *len);
hret_t ext2_readdir(struct file *this_file, struct dirent *result);
hret_t ext2_stat(struct inode * this_inode, struct stat *result);
hret_t ext2_truncate(struct inode *this_inode, off_t length);
hret_t ext2_inode_destructor(struct inode *to_del);
hret_t ext2_sync(struct inode *this_inode);

/* Fonctions utilitaire du driver ext2 */
hret_t ext2_read_group_descriptor(struct super_block *super, uint32_t index, struct ext2_group_descriptor **result);
hret_t ext2_write_group_descriptor(struct super_block *super, uint32_t index, struct ext2_group_descriptor *gdesc);
hret_t ext2_write_inode(struct super_block *super, uint32_t inode_nr, struct ext2_inode *to_write);
hret_t ext2_release_inode(struct super_block *super, uint32_t inode_id, struct ext2_inode *ext2_i);
hret_t ext2_read_inode(struct super_block *super, uint32_t inode_nr, struct ext2_inode **result);
hret_t ext2_get_vfs_inode(struct super_block *super, uint32_t inode_nr, struct inode **result);
hret_t ext2_read_vfs_direntry(struct inode *inode, uint32_t dirent_id, struct dirent *result);
hret_t ext2_get_inode_block(struct inode *i, uint32_t block_index, uint32_t *result_block_id);
hret_t ext2_init_dir(struct super_block *super, struct inode *dir, struct inode *parent);
hret_t ext2_get_free_inode(struct super_block *super, uint32_t *result_inode_id);
hret_t ext2_get_free_block(struct super_block *super, uint32_t *result_block_id);
hret_t ext2_write_block(struct inode *i, uint32_t block_index, char *buffer);
hret_t ext2_read_block(struct inode *i, uint32_t block_index, char **buffer);
bool_t ext2_name_match(const char *name1, const char *name2, int max_count);
hret_t ext2_write_super(struct ext2_memory_super *ext2_s, uint32_t dev);
hret_t ext2_release_block(struct super_block *super, uint32_t block_id);
hret_t ext2_set_inode_size(struct ext2_inode *ext2_i, uint32_t newsize);
enum inode_type ext2_convert_dirent_type(uint8_t ext2_dirent_type);
uint8_t ext2_dirent_convert_vfs_type(enum inode_type vfs_type);
uint16_t ext2_inode_convert_vfs_type(enum inode_type vfs_type);
hret_t ext2_add_block(struct inode *i, uint32_t block_index);

/* Fonction qui permet de débuguer le système de fichier */
void ext2_debugk(char *format, ...);

extern struct super_operation ext2_super_op;			// Opérations du super_block
extern struct file_system_operation ext2_fs_op;		    // Opérations du système de fichier

extern struct inode_operation ext2_inode_op;			// Opération pour les fichiers
extern struct inode_dir_operation ext2_dir_op;			// Opérations pour les répertoires

extern struct open_dir_operation ext2_open_dir_op;		// Opération pour un répertoire ouvert
extern struct open_file_operation ext2_open_file_op;	// Opération pour un fichier ouvert
