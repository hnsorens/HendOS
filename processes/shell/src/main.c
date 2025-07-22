#include <ctype.h>
#include <memory.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

typedef struct
{
    char** args;
    int count;
} arg_list_t;

typedef struct
{
    char* name;
    char* command;
} alias_t;

typedef struct
{
    alias_t* alias;
    uint64_t alias_count;
    uint64_t alias_capacity;
} alias_array_t;

typedef struct
{
    char** variables;
    uint64_t var_count;
    uint64_t var_capacity;
} variables_t;

variables_t variables;
variables_t env_variables;

alias_array_t alias_array;

int is_valid_var_assignment(const char* s)
{
    if (!s || !*s)
        return 0;

    const char* eq = strchr(s, '=');
    if (!eq || eq == s)
        return 0; // No '=', or empty name

    // Check NAME part (s to eq-1)
    for (const char* p = s; p < eq; ++p)
    {
        if (p == s)
        {
            // First character: must be letter or underscore
            if (!isalpha(*p) && *p != '_')
                return 0;
        }
        else
        {
            // Rest: must be letter, digit, or underscore
            if (!isalnum(*p) && *p != '_')
                return 0;
        }
    }

    // If you want to allow empty value (like FOO=), that's fine.
    // You could check eq[1] != '\0' if you want to disallow that.

    return 1;
}

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

int check_variables(const char* var_name, char** out)
{
    for (int i = 0; i < variables.var_count; i++)
    {
        size_t name_size = strlen(var_name);
        char* cmp_var = variables.variables[i];
        if (strncmp(var_name, cmp_var, name_size) == 0 && cmp_var[name_size] == '=')
        {
            *out = variables.variables[i];
            return 1;
        }
    }
    return 0;
}

void replace_first_char(char* str, char find, char replace)
{
    if (!str)
        return;

    for (; *str; ++str)
    {
        if (*str == find)
        {
            *str = replace;
            break; // only the first match
        }
    }
}

void export_var(char* var_name)
{
    for (int i = 0; i < variables.var_count; i++)
    {
        char* var = variables.variables[i];

        int name_match = 1;
        while (*var != '=')
        {
            if (!(*var_name))
                return;
            if (*var != *var_name)
                name_match = 0;
            var++;
            var_name++;
        }
        if (name_match)
        {
            if (env_variables.var_count == env_variables.var_capacity)
            {
                env_variables.var_capacity *= 2;
                env_variables.variables = realloc(env_variables.variables, env_variables.var_capacity * sizeof(char*));
            }

            env_variables.variables[env_variables.var_count++] = variables.variables[i];
            return;
        }
    }
}

void purge_var(char* var_name)
{
    for (int i = 0; i < variables.var_count; i++)
    {
        char* var = variables.variables[i];

        int name_match = 1;
        char* var_name_copy = var_name;
        while (*var != '=')
        {
            if (!(*var_name_copy))
                return;
            if (*var != *var_name_copy)
                name_match = 0;
            var++;
            var_name_copy++;
        }
        if (name_match)
        {
            for (int i2 = 0; i2 < env_variables.var_count; i2++)
            {
                var = env_variables.variables[i2];
                name_match = 1;
                while (*var != '=')
                {
                    if (!(*var_name))
                        return;
                    if (*var != *var_name)
                        name_match = 0;
                    var++;
                    var_name++;
                }
                if (name_match)
                {
                    env_variables.variables[i2] = env_variables.variables[--env_variables.var_count];
                }
            }
            free(variables.variables[i]);
            variables.variables[i] = variables.variables[--variables.var_count];
            return;
        }
    }
}

char* cwd;
int cwdLength;
char input[4096];

char* signal_to_string(uint64_t signal)
{
    switch (signal)
    {
    case 1: /* SIGHUP */
        return "Hangup";
    case 2: /* SIGINT */
        return "Interrupt";
    case 3: /* SIGQUIT */
        return "Quit";
    case 4: /* SIGILL */
        return "Illegal Instruction";
    case 5: /* SIGTRAP */
        return "Trace/breakpoint trap";
    case 6: /* SIGABRT */
        return "Aborted";
    case 7: /* SIGBUS */
        return "Buss Error";
    case 8: /* SIGFPE */
        return "Floating point exception";
    case 9: /* SIGKILL */
        return "Killed";
    case 10: /* SIGUSR1 */
        return "User defined signal 1";
    case 11: /* SIGSEGV */
        return "Segmentation fault";
    case 12: /* SIGUSR2 */
        return "User defined signal 2";
    case 13: /* SIGPIPE */
        return "Broken pipe";
    case 14: /* SIGALRM */
        return "Alarm clock";
    case 15: /* SIGTERM */
        return "Terminated";
    case 16: /* SIGSTKFLT */
        return "Stack fault";
    case 19: /* SIGSTOP */
        return "Stopped (signal)";
    case 20: /* SIGTSTP */
        return "Stopped (signal)";
    case 21: /* SIGTTN */
        return "Stopped (tty input)";
    case 22: /* SIGTTOU */
        return "Stopped (tty output)";
    case 23: /* SIGURG */
        return "Urgent condition on socket";
    case 24: /* SIGXCPU */
        return "CPU time limit exceeded";
    case 25: /* SIGXFSZ */
        return "File size limit exceeded";
    case 26: /* SIGVTALRM */
        return "Virtual alarm clock";
    case 27: /* SIGPROF */
        return "Profiling timer expired";
    case 30: /* SIGPWR */
        return "Power failure";
    case 31: /* SIGSYS */
        return "Bad system call";
    default:
        return "";
    }
}

void print_exit_status(uint64_t status)
{
    if (status == 0)
        return;
    uint64_t exit_code = (status >> 8) & 0xFF;
    uint64_t signal_num = status & 0x7F;
    uint64_t core_dumped = (status & 0x80) != 0;

    if (exit_code > 0)
    {
        printf("Exited with code %d\n", exit_code);
        return;
    }
    if (signal_num > 0)
    {
        printf("%s", signal_to_string(signal_num));
    }
    if (core_dumped)
    {
        printf(" (core dumped)");
    }
    printf("\n");
}

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
        return;
    }
    else if (strcmp(args.args[0], "echo") == 0)
    {
        if (args.count > 1)
        {
            printf("%s\n", input + 5);
            return;
        }
        else
        {
            return;
        }
    }
    else if (strcmp(args.args[0], "pwd") == 0)
    {
        if (args.count == 1)
        {
            printf("%s\n", cwd);
            return;
        }
        else
        {
            printf("Too many args for pwd command\n");
            return;
        }
    }
    else if (strcmp(args.args[0], "export") == 0)
    {
        if (args.count == 1)
        {
            for (int i = 0; i < env_variables.var_count; i++)
            {
                printf("%s\n", env_variables.variables[0]);
            }
            return;
        }
        else if (args.count == 2)
        {
            export_var(args.args[1]);
            return;
        }
        else
        {
            return;
        }
    }
    else if (strcmp(args.args[0], "exit") == 0)
    {
        exit(0);
    }
    else if (strcmp(args.args[0], "set") == 0)
    {
        if (args.count == 1)
        {
            for (int i = 0; i < variables.var_count; i++)
            {
                printf("%s\n", variables.variables[i]);
            }
            return;
        }
        else
        {
            return;
        }
    }
    else if (strcmp(args.args[0], "export") == 0)
    {
        if (args.count == 1)
        {
            for (int i = 0; i < variables.var_count; i++)
            {
                printf("%s\n", variables.variables[0]);
            }
            return;
        }
        else if (args.count == 2)
        {

            if (variables.var_capacity == variables.var_count)
            {
                variables.var_capacity *= 2;
                variables.variables = realloc(variables.variables, variables.var_capacity * sizeof(char*));
            }
            return;
        }
        else
        {
            printf("Too many args for export command\n");
            return;
        }
    }
    else if (strcmp(args.args[0], "unset") == 0)
    {
        if (args.count == 2)
        {
            purge_var(args.args[1]);
            return;
        }
        else if (args.count > 2)
        {
            printf("Too many args for unset command\n");
            return;
        }
        else
        {
            return;
        }
    }
    else if (strcmp(args.args[0], "alias") == 0)
    {
        if (args.count == 1)
        {
            for (int i = 0; i < alias_array.alias_count; i++)
            {
                alias_t* alias = &alias_array.alias[i];
                printf("alias %s \'%s\'\n", alias->name, alias->command);
            }
            return;
        }
        else if (args.count = 3)
        {
            char* alias_name = args.args[1];
            char* alias_command = args.args[2];

            for (int i = 0; i < alias_array.alias_count; i++)
            {
                if (strcmp(alias_array.alias[i].name, alias_name) == 0)
                {
                    printf("overwriting alias\n");
                    alias_array.alias[i].command = realloc(alias_array.alias[i].command, strlen(alias_command) + 1);
                    memcpy(alias_array.alias[i].command, alias_command, strlen(alias_command) + 1);
                    return;
                }
            }

            if (alias_array.alias_count == alias_array.alias_capacity)
            {
                alias_array.alias_capacity *= 2;
                alias_array.alias = realloc(alias_array.alias, alias_array.alias_capacity * sizeof(alias_t));
            }

            alias_t alias;
            alias.name = malloc(strlen(alias_name) + 1);
            alias.command = malloc(strlen(alias_command) + 1);
            memcpy(alias.name, alias_name, strlen(alias_name) + 1);
            memcpy(alias.command, alias_command, strlen(alias_command) + 1);
            alias_array.alias[alias_array.alias_count++] = alias;
            return;
        }
        else
        {
            printf("Alias no no\n");
            return;
        }
    }
    else if (strcmp(args.args[0], "unalias") == 0)
    {
        if (args.count == 2)
        {
            char* alias_name = args.args[1];

            for (int i = 0; i < alias_array.alias_count; i++)
            {
                if (strcmp(alias_array.alias[i].name, alias_name) == 0)
                {
                    free(alias_array.alias->command);
                    free(alias_array.alias->name);
                    alias_array.alias[i] = alias_array.alias[--alias_array.alias_count];
                }
            }
            return;
        }
        else
        {
            return;
        }
    }
    else if (args.count == 1 && is_valid_var_assignment(input))
    {
        replace_first_char(input, '=', 0);

        char* var;
        if (check_variables(input, &var))
        {
            char* new_var = realloc(var, strlen(args.args[0]) + 1);

            for (int i = 0; i < variables.var_count; i++)
            {
                if (var == variables.variables[i])
                {
                    variables.variables[i] = new_var;
                    var = new_var;
                }
            }
            if (!var)
            {
                printf("Failed to set variable\n");
                return;
            }
        }
        else
        {
            if (variables.var_capacity == variables.var_count)
            {
                variables.var_capacity *= 2;
                variables.variables = realloc(variables.variables, variables.var_capacity * sizeof(char*));
            }

            var = malloc(strlen(args.args[0]) + 1);

            variables.variables[variables.var_count++] = var;
        }
        memcpy(var, args.args[0], strlen(args.args[0]) + 1);

        return;
    }
    else if (args.count >= 1)
    {
        // TODO: fix echo (doesnt work with alias)
        for (int i = 0; i < alias_array.alias_count; i++)
        {
            alias_t* alias = &alias_array.alias[i];
            if (strcmp(alias->name, args.args[0]) == 0)
            {
                arg_list_t alias_args = splitArgs(alias->command);
                arg_list_t arg_list;
                arg_list.count = alias_args.count + args.count - 1;
                arg_list.args = malloc(arg_list.count * sizeof(char*));
                memcpy(arg_list.args, alias_args.args, alias_args.count * sizeof(char*));
                memcpy(arg_list.args + alias_args.count, &args.args[1], (args.count - 1) * sizeof(char*));
                execute(arg_list);
                free(arg_list.args);
                free_args(alias_args);
                return;
            }
        }

        uint64_t pid = fork();

        if (pid == 0)
        {
            setpgid(0, 0);
            tcsetpgrp(0, 0);
            execve(args.args[0], 0, 0);
        }
        uint64_t status;
        waitpid(pid, &status, 0);
        tcsetpgrp(0, 0);
        print_exit_status(status);
    }
}

int main()
{
    cwd = malloc(4096);
    getcwd(cwd, 4096);
    cwdLength = strlen(cwd);

    env_variables.var_capacity = 1;
    env_variables.var_count = 0;
    env_variables.variables = malloc(sizeof(char*));

    variables.var_capacity = 1;
    variables.var_count = 0;
    variables.variables = malloc(sizeof(char*));

    alias_array.alias_capacity = 1;
    alias_array.alias_count = 0;
    alias_array.alias = malloc(sizeof(alias_t));

    while (1)
    {

        printf("user@system:%s$ ", cwd);
        fgets(input, 4096, stdin);
        arg_list_t args = splitArgs(input);

        execute(args);
        free_args(args);
    }
}