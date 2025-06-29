/**
 * @file unistd.h
 */

#ifndef UNI_STD_H
#define UNI_STD_H

#include <stdint.h>
#include <types.h>

void dup2(void* old_df, void* new_fd);

ssize_t read(int fd, void* buf, size_t count);

ssize_t write(int fd, const void* buf, size_t count);

int close(int fd);

pid_t fork(void);

int getpgid(uint64_t pid);

int setpgid(uint64_t pid, uint64_t pgid);

int getsid(uint64_t pid);

int setsid(uint64_t pid, uint64_t pgid);

// int execve(const char* pathname, char* const argv[], char* const envp[]);
int execve(const char* name, int argc, char** argv);

int execvp(const char* name, int argc, char** argv);

unsigned int sleep(unsigned int seconds);

int pipe(int pipefd[2]);

int tcsetpgrp(uint64_t fd, uint64_t pgid);

int tcgetpgrp(uint64_t fd);

#endif