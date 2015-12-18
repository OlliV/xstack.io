#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define LOG_CRIT    '0'
#define LOG_ERR     '1'
#define LOG_WARN    '2'
#define LOG_INFO    '3'
#define LOG_DEBUG   '4'

#if XSTACK_LOGGING != 0
#define LOG(_level_, _format_, ...) do { \
        fprintf(stderr, "%c:%s: "_format_"\n", _level_, __func__, \
                ##__VA_ARGS__); \
    } while (0)
#else
#define LOG(_level_, _format_, ...)
#endif

#endif /* LOGGER_H */
