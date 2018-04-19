#include <i386/cpu.h>
#include <i386/idt.h>
#include <lib/stdio.h>
#include <i386/i8254.h>
#include <core/syscall/syscall.h>
#include <core/process/process.h>

extern void asm_SyscallCore(void);

void SyscallSetup(void)
{
	MakeIdtDescriptor(SYSCALL_VECTOR, (uint32_t)&asm_SyscallCore, 0x08, IDT_HANDLER_PRESENT | IDT_INT_GATE32 | IDT_USER_ACCES);
}

/*
 * C'est cette fonction qui est chargé de gérer les appels systèmes et de redirigé le flot d'éxécution
 * du noyau vers l'appel système correspondant et en lui passant les bons paramètres.
 */
int SyscallCore(struct cpu_state *regs)
{
	// FIXME: Faille Meltdown (Spectre n'est pas résolvable pour l'instant)

	hret_t ret = 0;

	// Actualise les registres sauvegardé du processus
	current->cpu_contexte = regs;

	switch (regs->eax)
	{
		case SYSCALL_EXIT:
			sys_exit((int)regs->ebx);
			panic("L'appel système exit (0x%02x) a échoué !", SYSCALL_EXIT);
			break;

		case SYSCALL_OPEN:
			ret = sys_open((const char *)regs->ebx, (uint32_t)regs->ecx);
			break;

		case SYSCALL_CLOSE:
			ret = sys_close((uint32_t)regs->ebx);
			break;

		case SYSCALL_MOUNT:
			ret = sys_mount((const char *)regs->ebx, (const char)regs->ecx, (const char *)regs->edx);
			break;

		case SYSCALL_UMOUNT:
			ret = sys_umount((const char)regs->ebx);
			break;

		case SYSCALL_READ:
			ret = sys_read((uint32_t)regs->ebx, (const char *)regs->ecx, (size_t)regs->edx);
			break;

		case SYSCALL_WRITE:
			ret = sys_write((uint32_t)regs->ebx, (const char *)regs->ecx, (size_t)regs->edx);
			break;

		case SYSCALL_SEEK:
			ret = sys_seek((uint32_t)regs->ebx, (off_t *)regs->ecx, (int)regs->edx);
			break;

		case SYSCALL_READDIR:
			ret = sys_readdir((uint32_t)regs->ebx, (struct dirent *)regs->ecx);
			break;

		case SYSCALL_FORK:
			ret = sys_fork();
			break;

		case SYSCALL_FORKEXEC:
			ret = sys_forkexec((const char *)regs);
			break;

		case SYSCALL_MKDIR:
			ret = sys_mkdir((const char *)regs->ebx);
			break;

		case SYSCALL_RMDIR:
			ret = sys_rmdir((const char *)regs->ebx);
			break;

		case SYSCALL_UNLINK:
			ret = sys_unlink((const char *)regs->ebx);
			break;

		case SYSCALL_EXEC:
			ret = sys_exec((const char *)regs->ebx, (char **)regs->ecx, (char **)regs->edx);
			break;

		case SYSCALL_MKNOD:
			ret = sys_mknod((const char *)regs->ebx, (mode_t)regs->ecx, (dev_t)regs->edx);
			break;

		case SYSCALL_IOCTL:
			ret = sys_ioctl((uint32_t)regs->ebx, (uint32_t)regs->ecx, (uint32_t)regs->edx);
			break;

		case SYSCALL_BRK:
			ret = sys_brk((void *)regs->ebx);
			break;

		case SYSCALL_CHDIR:
			ret = sys_chdir((const char *)regs->ebx);
			break;

		case SYSCALL_GETCWD:
			ret = sys_getcwd((char *)regs->ebx, (size_t)regs->ecx);
			break;

		case SYSCALL_DUP:
			ret = sys_dup((int)regs->ebx);
			break;

		case SYSCALL_DUP2:
			ret = sys_dup2((int)regs->ebx, (int)regs->ecx);
			break;

		case SYSCALL_GETPID:
			ret = sys_getpid();
			break;

		case SYSCALL_ISATTY:
			ret = sys_isatty((int)regs->ebx);
			break;

		case SYSCALL_TIME:
			ret = sys_time((time_t *)regs->ebx);
			break;
		
		case SYSCALL_WAITPID:
			ret = sys_waitpid((int)regs->ebx, (int *)regs->ecx);
			break;

		case SYSCALL_KILL:
			ret = sys_kill((int)regs->ebx, (int)regs->ecx);
			break;

		case SYSCALL_SIGACTION:
			ret = sys_sigaction((int)regs->ebx, (uint32_t *)regs->ecx, (uint32_t *)regs->edx);
			break;

		case SYSCALL_SIGRETURN:
			ret = sys_sigreturn();
			break;

		case SYSCALL_STAT:
			ret = sys_stat((const char *)regs->ebx, (struct stat *)regs->ecx);
			debugk("[STAT]: Return code is %i\n", ret);
			break;

		case SYSCALL_DEBUG:
			ret = sys_debug((int)regs->ebx, (const char *)regs->ecx);
			break;

		case SYSCALL_SYSCONF:
			ret = sys_sysconf((int)regs->ebx);
			break;

		case SYSCALL_ACCESS:
			ret = sys_access((const char *)regs->ebx, (int)regs->ecx);
			break;

		case SYSCALL_FSTAT:
			ret = sys_fstat((int)regs->ebx, (struct stat *)regs->ecx);
			break;

		case SYSCALL_FCNTL:
			ret = sys_fcntl((int)regs->ebx, (int)regs->ecx, (int)regs->edx);
			break;

		case SYSCALL_PIPE:
			ret = sys_pipe((int *)regs->ebx);
			break;
		
		default:
			debugk("[SYSCALL]: Unknown 0x%02x system call\n", regs->eax);
			return -ERR_NO_SYS;
			break;
	}

	return ret;
}