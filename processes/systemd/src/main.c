#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    // FILE* tty = fopen("/dev/tty0", 0);

    if (fork() == 0)
    {
        setpgid(0, 0);
        execve("shell", 0, 0);
    }

    while (1)
        ;
}