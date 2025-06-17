#include <stdio.h>
#include <unistd.h>

int main(int argc, char** argv)
{
    printf("Hello World!\n");

    printf("CURRENT PID: %d\n", 0);
    printf("CURRENT PGID: %d\n", getpgid(0));

    return 0;
}