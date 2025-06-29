#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>

int main()
{
    while (1)
    {
        printf(" _   _                _ _____ _____ \n");
        printf("| | | |              | |  _  /  ___|\n");
        printf("| |_| | ___ _ __   __| | | | \\ `--. \n");
        printf("|  _  |/ _ \\ '_ \\ / _` | | | |`--. \\\n");
        printf("| | | |  __/ | | | (_| \\ \\_/ /\\__/ /\n");
        printf("\\_| |_/\\___|_| |_|\\__,_|\\___/\\____/ \n");
        printf("    HendOS v0.1.0 | Terminal Interface\n");
        printf("--------------------------------------------------\n");
        printf(" Built on : May 11, 2025\n");
        printf(" Arch     : x86_64\n");
        printf("--------------------------------------------------\n");

        char username[64];
        char password[64];
        while (1)
        {

            printf("login: ");
            fgets(username, 64, stdin);
            printf("password: ");
            fgets(password, 64, stdin);
            if (strcmp(username, "root") == 0 && strcmp(password, "password") == 0)
            {
                break;
            }
            printf("Login incorrect.\n");
        }

        printf("Login successfull. Welcome %s!\n", username);

        uint64_t pid = fork();

        if (!pid)
        {
            setpgid(0, 0);
            tcsetpgrp(0, 0);
            execve("shell", 0, 0);
        }
        uint64_t status;
        waitpid(pid, &status, 0);
        tcsetpgrp(0, 0);
    }
}