#include <lib/stdio.h>
#include <i386/i8254.h>
#include <i386/paging.h>
#include <core/mm/phys.h>
#include <core/syscall/syscall.h>

#define _SC_ARG_MAX                       0
#define _SC_CHILD_MAX                     1
#define _SC_CLK_TCK                       2
#define _SC_NGROUPS_MAX                   3
#define _SC_OPEN_MAX                      4
#define _SC_JOB_CONTROL                   5
#define _SC_SAVED_IDS                     6
#define _SC_VERSION                       7
#define _SC_PAGESIZE                      8

#define _SC_PHYS_PAGES 11

int sys_sysconf(int name)
{
    switch(name)
    {
        case _SC_OPEN_MAX:
            return MAX_OPENED_FILE;

        case _SC_CLK_TCK:
            return PIT_8254_DEFAULT_FRENQUENCY;

        case _SC_PAGESIZE:
            return PAGE_SIZE;

        case _SC_PHYS_PAGES:
            return get_nb_physical_pages();

        default:
            debugk("sysconf(): unknow value 0x%08x (%u)", name, name);
            break;
    }

    return -ERR_BAD_ARG;
}
