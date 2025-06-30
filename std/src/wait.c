#include <syscall.h>
#include <wait.h>

__pid_t waitpid(__pid_t __pid, int* __stat_loc, int __options)
{
    return syscall(17, __pid, __stat_loc, __options);
}