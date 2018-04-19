#pragma once
#pragma GCC system_header			// Supprimer les warning (parfois justifié) de ce .h
#include <types.h>
#include <core/dev/dev.h>
#include <usr/libc/unistd.h>

#define NULL ((void *)0)

#define SEEK_SET	0				// Position à partir du début du fichier
#define SEEK_CUR	1				// Position à partir de la position courante
#define SEEK_END	2				// Position à partir de la fin du fichier

#define O_RDONLY	0x0
#define O_WRONLY	0x1
#define O_RDWR		0x2
#define O_CREAT		0x200
#define O_TRUNC		0x400
#define O_EXCL		0x800
#define O_DIRECTORY	0x10000

#define S_IFREG		0x01
#define S_IFBLK		0x02
#define S_IFDIR		0x04
#define S_IFCHR		0x08

// Nombre majeurs des périphériques caractères et bloc
#define TTY_MAJOR		1

#define DIRENT_MAX_NAME_SIZE		255

struct dir_struct;								// La structure est defini dans stdio.c: elle n'est pas directement accesible à l'utilisateur
typedef struct dir_struct DIR;					// On définit la structure répertoire

enum inode_type
{
	INODE_FILE = 1,
	INODE_DIRECTORY = 2,
	INODE_CHAR_DEVICE = 3,
	INODE_BLOCK_DEVICE = 4,
	INODE_SYMBOLIC_LINK = 5
};

struct dirent
{
	uint64_t inode_id;							// Identificateur de l'inode de l'entrée du répertoire
	uint32_t offset_in_dirfile;					// Offset dans le répertoire
	enum inode_type type;						// Type de l'inode
	char name[DIRENT_MAX_NAME_SIZE + 1];		// 255 caractères max sans le \0 de fin de chaîne de caractère
};

struct dir_struct
{
	int fd;										// Descripteur de fichier du répertoire
	struct dirent *entry;						// Entrée de répertoire
};


inline static int __attribute__((always_inline)) close(int fd)
{
	int retval;
	/*_asm {
		mov eax, SYSCALL_CLOSE
		mov ebx, fd
		int 0x80
		mov retval, eax
	}*/

	if (retval < 0)
		return retval;
	else
		return retval;
}

inline static int __attribute__((always_inline)) open(const char *pathname, int flags)
{
	int retval;
	int fd;

	asm("							\n\
		mov eax, %2					\n\
		mov ebx, %3					\n\
		mov ecx, %4					\n\
		int 0x80					\n\
		mov %0, eax				\n\
		mov %1, ebx" : "=m"(retval), "=m"(fd) : "i"(SYSCALL_OPEN), "m"(pathname), "m"(flags) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	if (retval < 0)
		return retval;
	else
		return fd;
}

inline static off_t __attribute__((always_inline)) seek(int fd, off_t offset, int whence)
{
	off_t *offset64_ptr = &offset;
	int offset_result;  
	int retval;

	/*_asm {
		mov eax, SYSCALL_SEEK
		mov ebx, fd
		mov ecx, offset64_ptr
		mov edx, whence
		int 0x80
		mov retval, eax
		mov offset_result, ebx
	}

	if (retval < 0)
		return retval;
	else
		return offset_result;*/
}

inline static int __attribute__((always_inline)) read(int fd, const void *buf, size_t count)
{
	int retval;
	int len_readed;

	/*_asm {
		mov eax, SYSCALL_READ
		mov ebx, fd
		mov ecx, buf
		mov edx, count
		int 0x80
		mov retval, eax
		mov len_readed, ebx
	}*/

	if (retval < 0)
		return retval;
	else
		return len_readed;
}

inline static int __attribute__((always_inline)) readdir(int fd, struct dirent *dirent)
{
	int retval;

	/*_asm {
		mov eax, SYSCALL_READDIR
		mov ebx, fd
		mov ecx, dirent
		int 0x80
		mov retval, eax
	}*/

	return retval;
}

inline static int __attribute__((always_inline)) write(int fd, const void *buf, size_t count)
{
	/*int retval;
	int len_writed;

	_asm {
		mov eax, SYSCALL_WRITE
		mov ebx, fd
		mov ecx, buf
		mov edx, count
		int 0x80
		mov retval, eax
		mov len_writed, ebx
	}

	if (retval < 0)
		return retval;
	else
		return len_writed;*/
}

inline static int __attribute__((always_inline)) mkdir(const char *pathname)
{
	int retval;

	asm("							\n\
		mov eax, %1					\n\
		mov ebx, %2					\n\
		int 0x80					\n\
		mov %0, eax" : "=m"(retval) : "i"(SYSCALL_MKDIR), "m"(pathname) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	if (retval < 0)
		return retval;
	else
		return 0;
}

inline static int __attribute__((always_inline)) rmdir(const char *pathname)
{
	/*int retval;

	_asm {
		mov eax, SYSCALL_RMDIR
		mov ebx, pathname
		int 0x80
		mov retval, eax
	}

	if (retval < 0)
		return retval;
	else
		return 0;*/
}


inline static int __attribute__((always_inline)) unlink(const char *pathname)
{
	/*int retval;

	_asm {
		mov eax, SYSCALL_UNLINK
		mov ebx, pathname
		int 0x80
		mov retval, eax
	}

	if (retval < 0)
		return retval;
	else
		return 0;*/
}

inline static int __attribute__((always_inline)) execve(const char *file, const char *argv[], const char *envp[])
{
	int retval;

	asm("							\n\
		mov eax, %1					\n\
		mov ebx, %2					\n\
		mov ecx, %3					\n\
		mov edx, %4					\n\
		int 0x80					\n\
		mov %0, eax" : "=m"(retval) : "i"(SYSCALL_EXEC), "m"(file), "m"(argv), "m"(envp) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	if (retval < 0)
		return retval;
	else
		return 0;
}

inline static int __attribute__((always_inline)) exec(const char *file)
{
	char *argv[2];
	argv[0] = (char *) file;
	argv[1] = NULL;

	execve(file, (const char **)argv, NULL);

	return -1;
}

inline static int __attribute__((always_inline)) mknod(const char *file, mode_t mode, dev_t dev)
{
	int retval;

	asm("mov eax, %1				\n\
	 	 mov ebx, %2				\n\
		 mov ecx, %3				\n\
		 mov edx, %4				\n\
		 int 0x80					\n\
		 mov %0, eax" : "=m"(retval) : "i"(SYSCALL_MKNOD), "m"(file), "m"(mode), "m"(dev) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	if (retval < 0)
		return retval;
	else
		return 0;
}

inline static int __attribute__((always_inline)) umount(char mountpoint)
{
	int retval;

	asm("mov eax, %1				\n\
	 	 mov ebx, %2				\n\
		 int 0x80					\n\
		 mov %0, eax" : "=m"(retval) : "i"(SYSCALL_UMOUNT), "m"(mountpoint) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	if (retval < 0)
		return retval;
	else
		return 0;
}


inline static int __attribute__((always_inline)) mount(char *dev, char mountpoint, char *filesystem)
{
	int retval;

	asm("mov eax, %1				\n\
		 mov ebx, %2				\n\
   		 mov ecx, %3				\n\
   		 mov edx, %4				\n\
   		 int 0x80					\n\
  		 mov %0, eax" : "=m"(retval) : "i"(SYSCALL_MOUNT), "m"(dev), "m"(mountpoint), "m"(filesystem) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	if (retval < 0)
		return retval;
	else
		return 0;
}

inline static int __attribute__((always_inline)) wait(int *status)
{
	int retval;

	asm("mov eax, %1				\n\
		 mov ebx, -1				\n\
	 	 mov ecx, %2				\n\
		 int 0x80					\n\
		 mov %0, eax" : "=m"(retval) : "i"(SYSCALL_WAITPID), "m"(status) : "memory", "eax", "ebx", "ecx", "edx", "edi", "esi");

	if (retval < 0)
		return -1;
	else
		return retval;
}