/**
 * @file unistd.h
 */

#ifndef UNI_STD_H
#define UNI_STD_H

#include <types.h>

ssize_t read(int fd, void* buf, size_t count);

ssize_t write(int fd, const void* buf, size_t count);

int close(int fd);

pid_t fork(void);

int execve(const char* pathname, char* const argv[], char* const envp[]);

unsigned int sleep(unsigned int seconds);

int pipe(int pipefd[2]);

#endif