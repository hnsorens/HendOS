#include <stdio.h>

int main(int argc, char* argv[])
{
    printf("ls?\n");
    if (argc > 1)
    {
        printf("file: %s\n", argv[1]);
    }
    else
    {
        printf("no args\n");
    }
}