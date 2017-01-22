/**
 *******************************************************************************
 * @file    dyndebug.c
 * @author  Olli Vanhoja
 * @brief   Dynamic debug messages.
 * @section LICENSE
 * Copyright (c) 2016 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "dyndebug.h"

#define __GLOBL1(sym) __asm__(".globl " #sym)
#define __GLOBL(sym) __GLOBL1(sym)

__GLOBL(__start_set_debug_msg_sect);
__GLOBL(__stop_set_debug_msg_sect);
extern struct _dyndebug_msg __start_set_debug_msg_sect;
extern struct _dyndebug_msg __stop_set_debug_msg_sect;

int toggle_dbgmsg(char * cfg)
{
    struct _dyndebug_msg * msg_opt = &__start_set_debug_msg_sect;
    struct _dyndebug_msg * stop = &__stop_set_debug_msg_sect;
    char strbuf[40];
    char * file = strbuf;
    char * line;

    if (msg_opt == stop)
        return -EINVAL;

    snprintf(strbuf, sizeof(strbuf), "%s", cfg);
    file = strbuf;
    line = strchr(strbuf, ':');

    if (line) {
        line[0] = '\0';
        line++;
    }

    while (msg_opt < stop) {
        if (strcmp(file, msg_opt->file) == 0) {
            if (line && *line != '\0') {
                char msgline[12];

                snprintf(msgline, sizeof(msgline), "%d", msg_opt->line);
                if (strcmp(line, msgline) != 0)
                    goto next;
            }
            msg_opt->flags ^= 1;
        }

next:
        msg_opt++;
    }

    return 0;
}
