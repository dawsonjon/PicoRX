/**
 * @file ring_buffer_lib.h
 * @brief A collection of multi-core safe functions for managing ring buffers
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
#ifndef RING_BUFFER_LIB_H
#define RING_BUFFER_LIB_H
#include <stdint.h>
#include "pico/stdlib.h"
#include "ring_buffer_lib_config.h"

// Define RING_BUFFER_SIZE_TYPE to the data type appropriate for storing
// the number of bytes in the buffer
// The default size can hold up to 255 entries.
#ifndef RING_BUFFER_SIZE_TYPE
#define RING_BUFFER_SIZE_TYPE uint8_t
#endif
#ifndef RING_BUFFER_MULTICORE_SUPPORT
#define RING_BUFFER_MULTICORE_SUPPORT 0
#endif
#if RING_BUFFER_MULTICORE_SUPPORT
#include "pico/critical_section.h"
#else
#include "pico/sync.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
#ifndef RING_BUFFER_ENTER_CRITICAL
#define RING_BUFFER_ENTER_CRITICAL(X) \
    uint32_t X; \
    do { X=save_and_disable_interrupts();} while(0)
#endif
#ifndef RING_BUFFER_EXIT_CRITICAL
#define RING_BUFFER_EXIT_CRITICAL(X) \
    do {restore_interrupts(X); } while(0)
#endif

/**
 * @struct a ring buffer
 */
typedef struct ring_buffer_s
{
    uint8_t *buf;       // A pointer to the ring buffer storage
    RING_BUFFER_SIZE_TYPE bufsize;      // the number of bytes in the buffer
    RING_BUFFER_SIZE_TYPE in_idx;       // the index into the buffer where the next data byte is written
    RING_BUFFER_SIZE_TYPE out_idx;      // the index into the buffer where the least recently written byte is stored
    RING_BUFFER_SIZE_TYPE num_buffered; // the number of bytes in the buffer
#if RING_BUFFER_MULTICORE_SUPPORT
    critical_section_t crit;            // a poiter to the critical section
#else
    uint32_t critical_section_data;     // data used by the critical section macros; application specific
#endif
} ring_buffer_t;

#if RING_BUFFER_MULTICORE_SUPPORT
/**
 * @brief initialize a ring buffer structure
 * @param ring_buf  a pointer to the ring buffer structure
 * @param buf       a pointer to the storage the ring buffer will use
 * @param buf_len   the number of bytes allocated for the buffer
 * @param lock_num  the critical section spinlock number to use for this ring buffer
 */
void ring_buffer_init(ring_buffer_t *ring_buf, uint8_t* buf, RING_BUFFER_SIZE_TYPE buf_len, uint lock_num);
#else
/**
 * @brief initialize a ring buffer structure
 * @param ring_buf  a pointer to the ring buffer structure
 * @param buf       a pointer to the storage the ring buffer will use
 * @param buf_len   the number of bytes allocated for the buffer
 * @param critical_section_data application specific data value used for managing critical sections (e.g. an IRQ number)
 */
void ring_buffer_init(ring_buffer_t *ring_buf, uint8_t* buf, RING_BUFFER_SIZE_TYPE buf_len, uint32_t critical_section_data);
#endif

/**
 * @brief put nvals bytes in a ring buffer without disabling interrupts
 * @param ring_buf pointer to the ring buffer structure
 * @param vals a buffer of bytes to put in the ring buffer
 * @param nvals the number of values to push
 * @returns the number of values pushed
 */
RING_BUFFER_SIZE_TYPE ring_buffer_push_unsafe(ring_buffer_t *ring_buf, const uint8_t* vals, RING_BUFFER_SIZE_TYPE nvals);

/**
 * @brief enter a critical section and then put byte in a ring buffer
 * @param ring_buf pointer to the ring buffer structure
 * @param val the byte to put in the ring buffer
 * @returns the number of values pushed
 */
RING_BUFFER_SIZE_TYPE ring_buffer_push(ring_buffer_t *ring_buf, const uint8_t* vals, RING_BUFFER_SIZE_TYPE nvals);

/**
 * @brief enter a critical section and then put byte in a ring buffer overwriting old data
 * @param ring_buf pointer to the ring buffer structure
 * @param val the byte to put in the ring buffer
 */
void ring_buffer_push_ovr(ring_buffer_t *ring_buf, const uint8_t *vals, RING_BUFFER_SIZE_TYPE nvals);

/**
 * @brief wait for the spinlock but do not disable interrupts before
 * getting the number of bytes currently stored in the buffer
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @return the number of bytes in the buffer (may be 0)
 */
RING_BUFFER_SIZE_TYPE ring_buffer_get_num_bytes_unsafe(ring_buffer_t *ring_buf);

/**
 * @brief wait for the spinlock and disable interrupts before
 * getting the number of bytes currently stored in the buffer
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @return the number of bytes in the buffer (may be 0)
 */
RING_BUFFER_SIZE_TYPE ring_buffer_get_num_bytes(ring_buffer_t *ring_buf);

/**
 * @brief wait for the spinlock but do not disable interrupts before
 * checking if the ring buffer is full
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @return true if the ring buffer is full
 */
bool ring_buffer_is_full_unsafe(ring_buffer_t *ring_buf);

/**
 * @brief wait for the spinlock and disable interrupts before
 * checking if the ring buffer is full
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @return true if the ring buffer is full
 */
bool ring_buffer_is_full(ring_buffer_t *ring_buf);

/**
 * @brief do not disable interrupts but do take spinlock
 * before checking if ring buffer is empty
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @return true if the ring buffer is empty
 */
bool ring_buffer_is_empty_unsafe(ring_buffer_t *ring_buf);

/**
 * @brief disable interrupts on this processor and wait for
 * spinlock before checking if the ring buffer is empty
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @return true if the ring buffer is empty
 */
bool ring_buffer_is_empty(ring_buffer_t *ring_buf);

/**
 * @brief read and remove a byte from the ring buffer without disabling interrupts
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @param vals a pointer to a byte buffer to hold the values read
 * @param maxvals the maximum number of values to read.
 * @return the number of bytes popped
 */
RING_BUFFER_SIZE_TYPE ring_buffer_pop_unsafe(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals);

/**
 * @brief entering a critical section, then read and remove a byte from the ring buffer
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @param vals a pointer to a byte buffer to hold the values read
 * @param maxvals the maximum number of values to read.
 * @return the number of bytes popped
 */
RING_BUFFER_SIZE_TYPE ring_buffer_pop(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals);

/**
 * @brief read bytes from the ring buffer without removing them from the buffer and without disabling interrupts
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @param vals a pointer to a byte buffer to hold the values read
 * @param maxvals the maximum number of values to read.
 * @return the number of bytes read into the vals buffer
 */
RING_BUFFER_SIZE_TYPE ring_buffer_peek_unsafe(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals);

/**
 * @brief enter a critical section, then read bytes from the ring buffer without removing it from the buffer
 * 
 * @param ring_buf a pointer to the ring buffer structure
 * @param vals a pointer to a byte buffer to hold the values read
 * @param maxvals the maximum number of values to read.
 * @return the number of bytes read into the vals buffer
 */
RING_BUFFER_SIZE_TYPE ring_buffer_peek(ring_buffer_t *ring_buf, uint8_t* vals, RING_BUFFER_SIZE_TYPE maxvals);
#ifdef __cplusplus
}
#endif
#endif //RING_BUFFER_LIB_H
