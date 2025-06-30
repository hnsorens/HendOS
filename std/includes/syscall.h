#ifndef __SYSCALL_H
#define __SYSCALL_H

long __syscall_ret(unsigned long r);

long syscall(long number, ...);

#endif