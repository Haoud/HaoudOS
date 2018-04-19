#pragma once
#include <types.h>
#include <i386/cpu.h>
#include <core/dev/dev.h>
#include <core/fs/inode.h>
#include <core/fs/dirent.h>

#define SYSCALL_VECTOR			0x80		// Pour la future compatibilité avec Linux

#define SYSCALL_RESERVED		0
#define SYSCALL_EXIT			1
#define SYSCALL_OPEN			2
#define SYSCALL_CLOSE			3
#define SYSCALL_MOUNT			4
#define SYSCALL_UMOUNT			5
#define SYSCALL_READ			6
#define SYSCALL_WRITE			7
#define SYSCALL_SEEK			8
#define SYSCALL_READDIR			9
#define SYSCALL_FORK			10
#define SYSCALL_FORKEXEC		11			// Mélange entre fork et exec (mais en plus optimisé), pas encore supporté
#define SYSCALL_MKDIR			12
#define SYSCALL_RMDIR			13
#define SYSCALL_UNLINK			14
#define SYSCALL_EXEC			15
#define SYSCALL_MKNOD			16
#define SYSCALL_IOCTL			17
#define SYSCALL_BRK				18
#define SYSCALL_CHDIR			19
#define SYSCALL_GETCWD			20
#define SYSCALL_DUP				21
#define SYSCALL_DUP2			22
#define SYSCALL_GETPID          23
#define SYSCALL_ISATTY          24
#define SYSCALL_TIME            25
#define SYSCALL_WAITPID         26
#define SYSCALL_KILL            27
#define SYSCALL_SIGACTION       28
#define SYSCALL_SIGRETURN       29
#define SYSCALL_STAT            30
#define SYSCALL_SYSCONF			31
#define SYSCALL_ACCESS			32
#define SYSCALL_FSTAT           33
#define SYSCALL_FCNTL           34
#define SYSCALL_PIPE            35

#define SYSCALL_DEBUG           666         // Appel système servant à envoyer des messages de debug au noyau (il decide ce qu'il doit en faire)

void SyscallSetup(void);
void SyscallHandler(void);
int SyscallCore(struct cpu_state *regs);

// Prototype des fonctions propres aux appels systèmes
void sys_exit(int exit_code);															// Permet à un programme de se terminer proprement
int sys_open(const char *path, uint32_t open_flags);				                    // Permet d'ouvrir un fichier (répertoire, fichier, périphériques...)
int sys_close(uint32_t fd);																// Ferme un fichier ouvert
int sys_mount(const char *source, const char drive_dst, const char *file_system);		// Mounte un système de fichier (à moitié supporté, très limité)
int sys_umount(const char drive_src);													// Permet de démonter un système de fichier
int sys_read(uint32_t fd, const char *buf, size_t len);				                    // Permet de lire un fichier ouvert(répertoire, fichier, périphériques...)
int sys_write(uint32_t fd, const char *buf, size_t len);			                    // Permet de d'écrire dans un fichier ouvert(répertoire, fichier, périphériques...)
int sys_seek(uint32_t fd, off_t *offset, int whence);						            // Permet de se positionner dans un fichier ouvert
int sys_readdir(uint32_t fd, struct dirent *direntry);									// Permet de "lire le contenu" d'un répertoire
int sys_fork(void);																		// Permet au processus de créer une copie conforme de lui même
int sys_forkexec(const char *file_path);												// Permet au processus d'éxécuter un executable qui deviendra son fils
int sys_mkdir(const char *pathname);													// Permet de crée un répertoire vide
int sys_rmdir(const char *pathname);													// Permet de supprimer un répertoire vide
int sys_unlink(const char *path);														// Permet de supprimer le lien avec un fichier (et de supprimer ce dernier si c'est le dernier lien)
int sys_exec(const char *path, char *argv[], char *env[]);								// Permet de remplacer le procesus courant par un autre processus
int sys_mknod(const char *path, mode_t mode, dev_t devid);								// Permet de créer un fichier de périphérique
int sys_ioctl(uint32_t fd, uint32_t request_id, uint32_t request_arg);					// Permet de modifier le comportement d'un pilote de pérphérique
int sys_brk(void *end);																	// Permet d'étendre (ou de réduire) le heap d'un processus
int sys_chdir(const char *path);														// Permet à un processus de changer son répertoire courant
int sys_getcwd(char *buffer, size_t buffer_size);										// Remplit le buffer avec le répertoire de travail du processus
int sys_dup(int oldfd);																	// Permet de dubliquer un descripteur de fichier
int sys_dup2(int oldfd, int newfd);														// Permet de dubliquer un descripteur de fichier en spécifiant le numéro du descripteur dupliqué
int sys_getpid(void);                                                                   // Retourner le PID du processus appelant
int sys_isatty(int fd);                                                                 // Permet de déterminer si un descripteur de fichier est un descripteur d'un terminal
time_t sys_time(time_t *tm);                                                            // Permet à un processus de récupérer des statitiques sur l'heure
int sys_waitpid(int pid, int *status);                                                  // Permet d'attendre la fin d'un processus fils
int sys_kill(int pid, int signal);                                                      // Permet d'envoyer un signal à un processu
int sys_sigreturn(void);                                                                // Pour le noyau seulement, ne doit pas être appelé par l'utilisateur
int sys_sigaction(int signum, uint32_t *action, uint32_t *old_action);                  // Change l'action d'un signal
int sys_stat(const char *path, struct stat *stat_struct);                               // Retourne différentes informations sur un fichier
int sys_sysconf(int name);                                                              // Permet de retourner certaines valeurs systèmes
int sys_access(const char *path, int access);                                           // Permet de tester l'accès (existence/lecture/écriture/éxécution) à un fichier
int sys_fstat(int fd, struct stat *stat_struct);                                        // Retourne différentes informations sur un fichier déjà ouvert
int sys_fcntl(int fd, int request_id, int request_arg);                                 // Permet de manipuler un descripteur de fichier  
int sys_pipe(int fd[2]);                                                                // Permet de créer un tube de communication anonyme 

int sys_debug(int gravity, const char *msg);                                            // Permet d'envoyer des messages de debug au noyau