/**
 * @file stdarg.h
 */

#ifndef STD_ARG_H
#define STD_ARG_H

typedef __builtin_va_list va_list;

#define va_start(ap, last_param) __builtin_va_start(ap, last_param)
#define va_arg(ap, type) __builtin_va_arg(ap, type)
#define va_end(ap) __builtin_va_end(ap)

#endif