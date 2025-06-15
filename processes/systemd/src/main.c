#include <stdlib.h>

int main()
{
    execve("shell", 0, 0);
    while (1)
        ;
}