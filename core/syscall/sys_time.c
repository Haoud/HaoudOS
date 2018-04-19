#include <core/time/time.h>

time_t sys_time(time_t *tm)
{
    *tm = get_current_unix_time();
    return *tm;
}