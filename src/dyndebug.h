/**
 *******************************************************************************
 * @file    dyndebug.h
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

#pragma once
#ifndef _DYNDEBUG_H_
#define _DYNDEBUG_H_

struct _dyndebug_msg {
    char flags;
    int line;
    const char * file;
};

int toggle_dbgmsg(char * cfg);

#define DYNDBUG_PRINT(_fn_, ...) do {                       \
    static struct _dyndebug_msg _dbg_msg                    \
        __attribute__((__section__("set_debug_msg_sect")))  \
        __attribute__((__used__))  =                        \
        { .flags = 0, .file = __FILE__, .line = __LINE__ }; \
    if (_dbg_msg.flags & 1) {                               \
        _fn_(__VA_ARGS__);                                  \
    }                                                       \
} while (0)

#endif /* !_DYNDEBUG_H_ */
