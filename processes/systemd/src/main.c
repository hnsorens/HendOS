#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    // if (fork() == 0)
    // {
    //     FILE* tty = fopen("/dev/vcon0", 0);

    //     dup2(tty, 0);
    //     dup2(tty, 1);
    //     dup2(tty, 2);

    //     fclose(tty);

    //     setsid(0, 0);
    //     setpgid(0, 0);
    //     tcsetpgrp(0, 0);

    //     execve("getty", 0, 0);
    // }

    while (1)
        ;
}