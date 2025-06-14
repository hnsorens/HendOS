#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    char** args;
    int count;
} arg_list_t;

int isSpace(char c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

char* copyToken(const char* start, int length)
{
    char* token = (char*)malloc(length + 1);
    for (int i = 0; i < length; ++i)
    {
        token[i] = start[i];
    }
    token[length] = '\0';
    return token;
}

arg_list_t splitArgs(const char* input)
{
    arg_list_t result = {NULL, 0};
    int capacity = 4;
    result.args = (char**)malloc(sizeof(char*) * capacity);

    const char* p = input;
    while (*p)
    {
        // Skip whitespace
        while (isSpace(*p))
            p++;
        if (*p == '\0')
            break;

        const char* start = p;
        char quote = 0;

        if (*p == '\'' || *p == '"')
        {
            // Quoted string
            quote = *p;
            start = ++p;
            while (*p && *p != quote)
                p++;
        }
        else
        {
            // Unquoted string
            while (*p && !isSpace(*p))
                p++;
        }

        int length = p - start;
        if (length > 0)
        {
            if (result.count >= capacity)
            {
                capacity *= 2;
                result.args = (char**)realloc(result.args, sizeof(char*) * capacity);
            }
            result.args[result.count++] = copyToken(start, length);
        }

        if (quote && *p == quote)
            p++; // Skip closing quote
    }

    return result;
}

void free_args(arg_list_t list)
{
    for (int i = 0; i < list.count; ++i)
    {
        free(list.args[i]);
    }
    free(list.args);
}

char* cwd;
int cwdLength;
char input[4096];

void execute(arg_list_t args)
{
    if (args.count == 0)
        return;
    if (strcmp(args.args[0], "cd") == 0)
    {
        if (args.count == 1)
            return;
        if (args.count > 2)
        {
            printf("Too many args for cd command\n");
            return;
        }
        chdir(args.args[1]);
        getcwd(cwd, 4096);
        cwdLength = strlen(cwd);
    }
    else if (strcmp(args.args[0], "echo") == 0)
    {
        if (args.count > 1)
        {
            printf("%s\n", input + 5);
        }
    }
    else
    {
        execve(args.args[0]);
    }
}

int main()
{
    cwd = malloc(4096);
    getcwd(cwd, 4096);
    cwdLength = strlen(cwd);
    while (1)
    {
        printf("user@system:%s$ ", cwd);
        fgets(input);
        arg_list_t args = splitArgs(input);

        execute(args);
        free_args(args);
    }
    exit(0);
}