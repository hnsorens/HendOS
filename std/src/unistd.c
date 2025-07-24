#include <syscall.h>
#include <unistd.h>

int dup2(int __fd, int __fd2)
{
    return syscall(13, __fd, __fd2);
}

ssize_t read(int __fd, void* __buf, size_t __nbytes)
{
    /* TODO: Implement read */
    return 0;
}

ssize_t write(int __fd, const void* __buf, size_t __n)
{
    /* TODO: Implement write */
    return 0;
}

int close(int __fd)
{
    /* TODO: implement close */
    return 0;
}

__pid_t fork(void)
{
    return syscall(8);
}

__pid_t getpid(void) {}

int setpgid(__pid_t __pid, __pid_t __pgid)
{
    return syscall(11, __pid, __pgid);
}

__pid_t getsid(__pid_t __pid)
{
    return syscall(19, __pid);
}

int execve(const char* __path, char* const __argv[], char* const __envp[])
{
    return syscall(2, __path, __argv, sizeof(__argv));
}

unsigned int sleep(unsigned int __seconds)
{
    return 0;
}

int pipe(int __pipedes[2])
{
    return 0;
}

int tcsetpgrp(int __fd, __pid_t __pgrp_id)
{
    return syscall(15, __fd, __pgrp_id);
}

__pid_t tcgetpgrp(int __fd)
{
    return syscall(16, __fd);
}

char* getcwd(char* __buf, size_t __size)
{
    return (char*)syscall(6, (long)__buf, __size);
}

int chdir(const char* __path)
{
    return syscall(5, __path);
}