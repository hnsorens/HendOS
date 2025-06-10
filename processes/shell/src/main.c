#include <stdio.h>
#include <stdlib.h>

int main()
{
    while (1)
    {
        printf("\nuser@system:$ ");
        char buffer[512];
        for (int i = 0; i < 512; i++)
        {
            buffer[i] = ' ';
        }
        scanf("%s", buffer);
        printf("\n%s", buffer);
    }
    exit(0);
}