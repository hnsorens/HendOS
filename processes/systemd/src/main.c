#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    if (fork())
    {
        setpgid(0, 0);

        execve("helloworld", 0, 0);
    }

    while (1)
        ;
}