/**
 * @file stats.h
 */

#ifndef STATS_H
#define STATS_H

#include <types.h>

typedef struct stat
{

} stat;

int stat(const char* pathname, struct stat* statbuf);

int fstat(int fd, struct stat* statbuf);

int mkdir(const char* pathname, mode_t mode);

#endif