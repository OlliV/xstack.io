/**
 *******************************************************************************
 * @file    queue_r.c
 * @author  Olli Vanhoja
 * @brief   Thread-safe queue.
 * @section LICENSE
 * Copyright (c) 2013 - 2015, 2017 Olli Vanhoja <olli.vanhoja@cs.helsinki.fi>
 * Copyright (c) 2012, 2013 Ninjaware Oy
 *                          Olli Vanhoja <olli.vanhoja@ninjaware.fi>
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

#include <stdint.h>
#include <string.h>

#include "queue_r.h"

queue_cb_t queue_create(size_t block_size, size_t array_size)
{
    queue_cb_t cb = {
        .b_size = block_size,
        .a_len = array_size / block_size,
        .m_read = 0,
        .m_write = 0
    };

    return cb;
}

int queue_alloc(queue_cb_t * cb)
{
    const size_t write = cb->m_write;
    const size_t next_element = (write + 1) % cb->a_len;
    const size_t b_size = cb->b_size;

    /* Check that the queue is not full */
    if (next_element == cb->m_read)
        return -1;

    return write * b_size;
}

void queue_commit(queue_cb_t * cb)
{
    const size_t next_element = (cb->m_write + 1) % cb->a_len;

    cb->m_write = next_element;
}

int queue_peek(queue_cb_t * cb, int * index)
{
    const size_t read = cb->m_read;
    const size_t b_size = cb->b_size;

    /* Check that the queue is not empty */
    if (read == cb->m_write)
        return 0;

    *index = read * b_size;

    return 1;
}

int queue_discard(queue_cb_t * cb, size_t n)
{
    size_t count;

    for (count = 0; count < n; count++) {
        const size_t read = cb->m_read;

        /* Check that the queue is not empty */
        if (read == cb->m_write)
            break;

        cb->m_read = (read + 1) % cb->a_len;
    }

    return count;
}

void queue_clear_from_push_end(queue_cb_t * cb)
{
    cb->m_write = cb->m_read;
}

void queue_clear_from_pop_end(queue_cb_t * cb)
{
    cb->m_read = cb->m_write;
}

int queue_isempty(queue_cb_t * cb)
{
    return (int)(cb->m_write == cb->m_read);
}

int queue_isfull(queue_cb_t * cb)
{
    return (int)(((cb->m_write + 1) % cb->a_len) == cb->m_read);
}
