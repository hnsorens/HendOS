#include <stdio.h>
#include <stdlib.h>

int main()
{
    while (1)
    {
        char buffer[512];
        printf("user@system:$ ");
        for (int i = 0; i < 512; i++)
        {
            buffer[i] = ' ';
        }
        fgets(buffer);
        printf("%s\n", buffer);
    }
    exit(0);
}