#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    // execve("shell", 0, 0);
    // while (1)
    //     ;
    int i = 1;
    if (fork())
    {
        execve("shell", 0, 0);
    }

    while (1)
        ;
}