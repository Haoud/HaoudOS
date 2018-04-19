#pragma once

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