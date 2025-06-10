/**
 * @file time.h
 */

#ifndef TIME_H
#define TIME_H

typedef int time_t;
typedef int clock_t;

time_t time(time_t* t);

clock_t clock(void);

unsigned int sleep(unsigned int seconds);

#endif