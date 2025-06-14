#include <stdio.h>

int main()
{
    const char* str = "Hello World\n";
    for (int i = 0; i < 10; i++)
    {
        printf("%d\n", i);
    }
    printf("%d %s\n", 10, str);

    char name[100];
    int age;
    char ch;

    scanf("%s %d %c", name, &age, &ch);
    printf("\nYour name is %s and you are %d years old and your character is %c", name, age, ch);

    return 0;
}