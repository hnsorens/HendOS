#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
    char* cwd = malloc(4096);
    getcwd(cwd, 512);
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