/**
 * @file fcntl.h
 */

#ifndef FCNTL_H
#define FCNTL_H

#define O_RDONLY
#define O_WRONLY
#define O_RDWR
#define O_CREAT
#define O_TRUNC
#define O_APPEND
// more if needed

int open(const char* pathname, int flags, ...);

int fcntl(int fd, int cmd, ...);

#endif