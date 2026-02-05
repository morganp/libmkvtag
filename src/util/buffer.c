/*
 * buffer.c - Dynamic buffer management
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "buffer.h"
#include "../include/mkvtag/mkvtag_error.h"

#include <stdlib.h>
#include <string.h>

#define BUFFER_DEFAULT_CAPACITY 256
#define BUFFER_GROWTH_FACTOR 2

int buffer_init(dyn_buffer_t *buf, size_t initial_capacity) {
    if (!buf) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (initial_capacity == 0) {
        initial_capacity = BUFFER_DEFAULT_CAPACITY;
    }

    buf->data = malloc(initial_capacity);
    if (!buf->data) {
        buf->size = 0;
        buf->capacity = 0;
        return MKVTAG_ERR_NO_MEMORY;
    }

    buf->size = 0;
    buf->capacity = initial_capacity;
    return MKVTAG_OK;
}

void buffer_free(dyn_buffer_t *buf) {
    if (buf) {
        free(buf->data);
        buf->data = NULL;
        buf->size = 0;
        buf->capacity = 0;
    }
}

int buffer_reserve(dyn_buffer_t *buf, size_t additional) {
    if (!buf) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    size_t needed = buf->size + additional;
    if (needed <= buf->capacity) {
        return MKVTAG_OK;
    }

    /* Grow by at least the growth factor */
    size_t new_capacity = buf->capacity * BUFFER_GROWTH_FACTOR;
    if (new_capacity < needed) {
        new_capacity = needed;
    }

    uint8_t *new_data = realloc(buf->data, new_capacity);
    if (!new_data) {
        return MKVTAG_ERR_NO_MEMORY;
    }

    buf->data = new_data;
    buf->capacity = new_capacity;
    return MKVTAG_OK;
}

int buffer_append(dyn_buffer_t *buf, const void *data, size_t size) {
    if (!buf) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (size == 0) {
        return MKVTAG_OK;
    }

    if (!data) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    int err = buffer_reserve(buf, size);
    if (err != MKVTAG_OK) {
        return err;
    }

    memcpy(buf->data + buf->size, data, size);
    buf->size += size;
    return MKVTAG_OK;
}

int buffer_append_byte(dyn_buffer_t *buf, uint8_t byte) {
    return buffer_append(buf, &byte, 1);
}

void buffer_clear(dyn_buffer_t *buf) {
    if (buf) {
        buf->size = 0;
    }
}

uint8_t *buffer_detach(dyn_buffer_t *buf, size_t *size) {
    if (!buf) {
        if (size) *size = 0;
        return NULL;
    }

    uint8_t *data = buf->data;
    if (size) {
        *size = buf->size;
    }

    buf->data = NULL;
    buf->size = 0;
    buf->capacity = 0;

    return data;
}
