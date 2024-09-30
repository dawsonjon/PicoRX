/**
 * @file ring_buffer_lib.c
 * @brief A collection of functions for managing ring buffers
 * @note This library depends on the Raspberry Pi Pico SDK for
 * managing critical sections
 * 
 * MIT License

 * Copyright (c) 2022 rppicomidi

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 */

#include "pico/assert.h"
#include "ring_buffer_lib.h"

#if RING_BUFFER_MULTICORE_SUPPORT
void ring_buffer_init(ring_buffer_t *ring_buf, uint8_t* buf, RING_BUFFER_SIZE_TYPE buf_len, uint lock_num)
#else
void ring_buffer_init(ring_buffer_t *ring_buf, uint8_t* buf, RING_BUFFER_SIZE_TYPE buf_len, uint32_t critical_section_data)
#endif
{
    assert(ring_buf);
    assert(buf);
    assert(buf_len > 0);
    ring_buf->buf = buf;
    ring_buf->bufsize = buf_len;
    ring_buf->in_idx = 0;
    ring_buf->out_idx = 0;
    ring_buf->num_buffered = 0;
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_init_with_lock_num(&ring_buf->crit, lock_num);
#else
    ring_buf->critical_section_data = critical_section_data;
#endif
}

static RING_BUFFER_SIZE_TYPE ring_buffer_push_core(ring_buffer_t *ring_buf, const uint8_t *vals, RING_BUFFER_SIZE_TYPE nvals)
{
    assert(ring_buf);
    assert(vals);
    RING_BUFFER_SIZE_TYPE npushed = 0;
    for (RING_BUFFER_SIZE_TYPE idx = 0; ring_buf->num_buffered < ring_buf->bufsize && idx < nvals; idx++) {
        ring_buf->buf[ring_buf->in_idx] = vals[idx];
        ring_buf->in_idx = ((ring_buf->in_idx+1) % ring_buf->bufsize);
        ++ring_buf->num_buffered;
        ++npushed;
    }
    return npushed;
}

static void ring_buffer_push_ovr_core(ring_buffer_t *ring_buf, const uint8_t *vals, RING_BUFFER_SIZE_TYPE nvals)
{
    assert(ring_buf);
    assert(vals);
    for (RING_BUFFER_SIZE_TYPE idx = 0; idx < nvals; idx++) {
        ring_buf->buf[ring_buf->in_idx] = vals[idx];
        ring_buf->in_idx = ((ring_buf->in_idx+1) % ring_buf->bufsize);
        if (ring_buf->num_buffered < ring_buf->bufsize)
        {
            ++ring_buf->num_buffered;
        } else {
            // buffer full
            ring_buf->out_idx = ring_buf->in_idx;
        }
    }
}

RING_BUFFER_SIZE_TYPE ring_buffer_push_unsafe(ring_buffer_t *ring_buf, const uint8_t *vals, RING_BUFFER_SIZE_TYPE nvals)
{
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_lock_unsafe_blocking(ring_buf->crit.spin_lock);
#endif
    RING_BUFFER_SIZE_TYPE result =  ring_buffer_push_core(ring_buf, vals, nvals);
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_unlock_unsafe(ring_buf->crit.spin_lock);
#endif
    return result;
}

RING_BUFFER_SIZE_TYPE ring_buffer_push(ring_buffer_t *ring_buf, const uint8_t *vals, RING_BUFFER_SIZE_TYPE nvals)
{
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_enter_blocking(&ring_buf->crit);
#else
    RING_BUFFER_ENTER_CRITICAL(status);
#endif
    RING_BUFFER_SIZE_TYPE result = ring_buffer_push_core(ring_buf, vals, nvals);
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_exit(&ring_buf->crit);
#else
    RING_BUFFER_EXIT_CRITICAL(status);
#endif
    return result;
}

void ring_buffer_push_ovr(ring_buffer_t *ring_buf, const uint8_t *vals, RING_BUFFER_SIZE_TYPE nvals)
{
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_enter_blocking(&ring_buf->crit);
#else
    RING_BUFFER_ENTER_CRITICAL(status);
#endif
    ring_buffer_push_ovr_core(ring_buf, vals, nvals);
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_exit(&ring_buf->crit);
#else
    RING_BUFFER_EXIT_CRITICAL(status);
#endif
}

// TODO: are these functions really necessary?
RING_BUFFER_SIZE_TYPE ring_buffer_get_num_bytes_unsafe(ring_buffer_t *ring_buf)
{
    assert(ring_buf);
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_lock_unsafe_blocking(ring_buf->crit.spin_lock);
#endif
    RING_BUFFER_SIZE_TYPE num_bytes = ring_buf->num_buffered;
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_unlock_unsafe(ring_buf->crit.spin_lock);
#endif
    return num_bytes;
}

RING_BUFFER_SIZE_TYPE ring_buffer_get_num_bytes(ring_buffer_t *ring_buf)
{
    assert(ring_buf);
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_enter_blocking(&ring_buf->crit);
#else
    RING_BUFFER_ENTER_CRITICAL(status);
#endif

    RING_BUFFER_SIZE_TYPE num_bytes = ring_buf->num_buffered;

#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_exit(&ring_buf->crit);
#else
    RING_BUFFER_EXIT_CRITICAL(status);
#endif
    return num_bytes;
}

bool ring_buffer_is_full_unsafe(ring_buffer_t *ring_buf)
{
    assert(ring_buf);
    return ring_buffer_get_num_bytes_unsafe(ring_buf) == ring_buf->bufsize;
}

bool ring_buffer_is_full(ring_buffer_t *ring_buf)
{
    assert(ring_buf);
    return ring_buffer_get_num_bytes(ring_buf) == ring_buf->bufsize;
}

bool ring_buffer_is_empty_unsafe(ring_buffer_t *ring_buf)
{
    RING_BUFFER_SIZE_TYPE result = ring_buffer_get_num_bytes_unsafe(ring_buf);
    return result == 0;
}

bool ring_buffer_is_empty(ring_buffer_t *ring_buf)
{
    RING_BUFFER_SIZE_TYPE result = ring_buffer_get_num_bytes(ring_buf);
    return result == 0;
}

static RING_BUFFER_SIZE_TYPE ring_buffer_pop_core(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals)
{
    assert(ring_buf);
    assert(vals);
    RING_BUFFER_SIZE_TYPE npopped = 0;
    while (ring_buf->num_buffered > 0 && npopped < maxvals) {
        vals[npopped++] = ring_buf->buf[ring_buf->out_idx];
        ring_buf->out_idx = (ring_buf->out_idx+1) % ring_buf->bufsize;
        --ring_buf->num_buffered;
    }
    return npopped;
}

RING_BUFFER_SIZE_TYPE ring_buffer_pop_unsafe(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals)
{
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_lock_unsafe_blocking(ring_buf->crit.spin_lock);
#endif
    RING_BUFFER_SIZE_TYPE result =  ring_buffer_pop_core(ring_buf, vals, maxvals);
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_unlock_unsafe(ring_buf->crit.spin_lock);
#endif
    return result;
}

RING_BUFFER_SIZE_TYPE ring_buffer_pop(ring_buffer_t *ring_buf, uint8_t *vals, RING_BUFFER_SIZE_TYPE maxvals)
{
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_enter_blocking(&ring_buf->crit);
#else
    RING_BUFFER_ENTER_CRITICAL(status);
#endif
    RING_BUFFER_SIZE_TYPE result = ring_buffer_pop_core(ring_buf, vals, maxvals);
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_exit(&ring_buf->crit);
#else
    RING_BUFFER_EXIT_CRITICAL(status);
#endif
    return result;
}

static RING_BUFFER_SIZE_TYPE ring_buffer_peek_core(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals)
{
    assert(ring_buf);
    assert(vals);
    RING_BUFFER_SIZE_TYPE npopped = 0;
    RING_BUFFER_SIZE_TYPE out_idx = ring_buf->out_idx;
    while (ring_buf->num_buffered > 0 && npopped < maxvals) {
        vals[npopped++] = ring_buf->buf[out_idx];
        out_idx = (out_idx+1) % ring_buf->bufsize;
    }
    return npopped;
}

RING_BUFFER_SIZE_TYPE ring_buffer_peek_unsafe(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals)
{
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_lock_unsafe_blocking(ring_buf->crit.spin_lock);
#endif
    RING_BUFFER_SIZE_TYPE result =  ring_buffer_peek_core(ring_buf, vals, maxvals);
#if RING_BUFFER_MULTICORE_SUPPORT
    spin_unlock_unsafe(ring_buf->crit.spin_lock);
#endif
    return result;
}

RING_BUFFER_SIZE_TYPE ring_buffer_peek(ring_buffer_t *ring_buf, uint8_t *vals, RING_BUFFER_SIZE_TYPE maxvals)
{
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_enter_blocking(&ring_buf->crit);
#else
    RING_BUFFER_ENTER_CRITICAL(status);
#endif
    RING_BUFFER_SIZE_TYPE result = ring_buffer_peek_core(ring_buf, vals, maxvals);
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_exit(&ring_buf->crit);
#else
    RING_BUFFER_EXIT_CRITICAL(status);
#endif
    return result;
}
