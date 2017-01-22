#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>

#include "dyndebug.h"

#define LOG_CRIT    '0'
#define LOG_ERR     '1'
#define LOG_WARN    '2'
#define LOG_INFO    '3'
#define LOG_DEBUG   '4'

#if XSTACK_LOGGING != 0
#define LOG(_level_, _fmt_, ...) do {                   \
    DYNDBUG_PRINT(fprintf, stderr, "%c:%s: "_fmt_,      \
                  _level_, __func__, ##__VA_ARGS__);    \
} while (0)
#else
#define LOG(_level_, _fmt_, ...)
#endif

#endif /* LOGGER_H */
