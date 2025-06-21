#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    while (1)
        ;
    if (fork() == 0)
    {
        FILE* tty = fopen("/dev/vcon0", 0);

        printf("THis file %d\n", tty);

        dup2(tty, 0);
        dup2(tty, 1);
        dup2(tty, 2);

        fclose(tty);

        setpgid(0, 0);
        execve("shell", 0, 0);
    }

    while (1)
        ;
}