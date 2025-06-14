#include <stdio.h>
#include <stdlib.h>

int main()
{
    char path[512];
    getcwd(path, 512);
    printf("%s\n", path);
    chdir("../notfolder/asd");
    getcwd(path, 512);
    printf("%s\n", path);
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