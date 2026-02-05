/*
 * buffer.h - Dynamic buffer management
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Dynamic byte buffer with automatic growth.
 */
typedef struct dyn_buffer {
    uint8_t *data;      /* Buffer data */
    size_t size;        /* Current data size */
    size_t capacity;    /* Allocated capacity */
} dyn_buffer_t;

/**
 * Initialize a buffer.
 *
 * @param buf The buffer to initialize
 * @param initial_capacity Initial capacity (0 for default)
 * @return 0 on success, negative error code on failure
 */
int buffer_init(dyn_buffer_t *buf, size_t initial_capacity);

/**
 * Free buffer resources.
 *
 * @param buf The buffer to free
 */
void buffer_free(dyn_buffer_t *buf);

/**
 * Ensure the buffer has at least the given additional capacity.
 *
 * @param buf The buffer
 * @param additional Additional bytes needed
 * @return 0 on success, negative error code on failure
 */
int buffer_reserve(dyn_buffer_t *buf, size_t additional);

/**
 * Append data to the buffer.
 *
 * @param buf The buffer
 * @param data Data to append
 * @param size Size of data to append
 * @return 0 on success, negative error code on failure
 */
int buffer_append(dyn_buffer_t *buf, const void *data, size_t size);

/**
 * Append a single byte to the buffer.
 *
 * @param buf The buffer
 * @param byte The byte to append
 * @return 0 on success, negative error code on failure
 */
int buffer_append_byte(dyn_buffer_t *buf, uint8_t byte);

/**
 * Clear the buffer without freeing memory.
 *
 * @param buf The buffer
 */
void buffer_clear(dyn_buffer_t *buf);

/**
 * Detach the buffer data. The caller takes ownership.
 * The buffer is reset to empty after this call.
 *
 * @param buf The buffer
 * @param size Output: size of the returned data
 * @return Pointer to the data, or NULL if buffer was empty
 */
uint8_t *buffer_detach(dyn_buffer_t *buf, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* BUFFER_H */
