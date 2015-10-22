#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#define LOG_CRIT    '0'
#define LOG_ERR     '1'
#define LOG_WARN    '2'
#define LOG_INFO    '3'
#define LOG_DEBUG   '4'

#define LOG(_level_, _format_, ...) do { \
        fprintf(stderr, "%c:%s: "_format_"\n", _level_, __func__, \
                ##__VA_ARGS__); \
    } while (0)

#endif /* LOGGER_H */
