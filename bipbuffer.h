#pragma once

#include "pico/critical_section.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
    unsigned long int size;

    /* region A */
    unsigned int a_start, a_end;

    /* region B */
    unsigned int b_end;

    /* is B inuse? */
    int b_inuse;
    critical_section_t crit;
    unsigned char data[];
} bipbuf_t;

/**
 * Create a new bip buffer.
 *
 * malloc()s space
 *
 * @param[in] size The size of the buffer */
bipbuf_t *bipbuf_new(const unsigned int size);

/**
 * Free the bip buffer */
void bipbuf_free(bipbuf_t *me);

/**
 * @param[in] data The data to be offered to the buffer
 * @param[in] size The size of the data to be offered
 * @return number of bytes offered */
int bipbuf_offer(bipbuf_t *me, const unsigned char *data, const int size);

/**
 * Look at data. Don't move cursor
 *
 * @param[in] len The length of the data to be peeked
 * @return data on success, NULL if we can't peek at this much data */
unsigned char *bipbuf_peek(bipbuf_t* me, const unsigned int len);

/**
 * Get pointer to data to read. Move the cursor on.
 *
 * @param[in] len The length of the data to be polled
 * @return pointer to data, NULL if we can't poll this much data */
unsigned char *bipbuf_poll(bipbuf_t* me, const unsigned int size);

/**
 * @return the size of the bipbuffer */
int bipbuf_size(bipbuf_t* me);

/**
 * @return 1 if buffer is empty; 0 otherwise */
int bipbuf_is_empty(bipbuf_t* me);

/**
 * @return how much space we have assigned */
int bipbuf_used(bipbuf_t* cb);

/**
 * @return bytes of unused space */
int bipbuf_unused(bipbuf_t* me);

#ifdef __cplusplus
}
#endif
