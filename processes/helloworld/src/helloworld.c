#include <stdio.h>

int main()
{
    const char* str = "Hello World\n";
    for (int i = 0; i < 10; i++)
    {
        printf("%d\n", i);
    }
    printf("%d %s\n", 10, str);
    exit(0);

    return 0;
}