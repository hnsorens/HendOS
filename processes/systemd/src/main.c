#include <stdlib.h>

int main()
{
    execve("shell");
    while (1)
        ;
}