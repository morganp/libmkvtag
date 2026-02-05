/*
 * ebml_writer.c - EBML element writing/serialization
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "ebml_writer.h"
#include "ebml_vint.h"
#include "ebml_ids.h"
#include "../include/mkvtag/mkvtag_error.h"

#include <string.h>

/**
 * Get the number of bytes needed to represent a uint value.
 */
static int uint_data_size(uint64_t value) {
    if (value == 0) return 0;
    if (value <= 0xFF) return 1;
    if (value <= 0xFFFF) return 2;
    if (value <= 0xFFFFFF) return 3;
    if (value <= 0xFFFFFFFF) return 4;
    if (value <= 0xFFFFFFFFFFULL) return 5;
    if (value <= 0xFFFFFFFFFFFFULL) return 6;
    if (value <= 0xFFFFFFFFFFFFFFULL) return 7;
    return 8;
}

/**
 * Write an element ID to a buffer.
 */
static int write_id(dyn_buffer_t *buf, uint32_t id) {
    uint8_t id_bytes[4];
    int id_len;
    int err = ebml_id_encode(id, id_bytes, &id_len);
    if (err != MKVTAG_OK) return err;
    return buffer_append(buf, id_bytes, id_len);
}

/**
 * Write a VINT-encoded size to a buffer.
 */
static int write_size(dyn_buffer_t *buf, uint64_t size) {
    uint8_t size_bytes[8];
    int size_len;
    int err = ebml_vint_encode(size, size_bytes, &size_len);
    if (err != MKVTAG_OK) return err;
    return buffer_append(buf, size_bytes, size_len);
}

int ebml_write_master_element_header(dyn_buffer_t *buf, uint32_t id,
                                     uint64_t content_size) {
    if (!buf) return MKVTAG_ERR_INVALID_ARG;

    int err = write_id(buf, id);
    if (err != MKVTAG_OK) return err;

    return write_size(buf, content_size);
}

int ebml_write_uint_element(dyn_buffer_t *buf, uint32_t id, uint64_t value) {
    if (!buf) return MKVTAG_ERR_INVALID_ARG;

    int data_size = uint_data_size(value);

    /* Special case: TargetTypeValue defaults to 0, but we want at least 1 byte */
    if (data_size == 0) {
        data_size = 1;
    }

    int err = write_id(buf, id);
    if (err != MKVTAG_OK) return err;

    err = write_size(buf, data_size);
    if (err != MKVTAG_OK) return err;

    /* Write value in big-endian */
    uint8_t data[8];
    for (int i = data_size - 1; i >= 0; i--) {
        data[i] = (uint8_t)(value & 0xFF);
        value >>= 8;
    }

    return buffer_append(buf, data, data_size);
}

int ebml_write_int_element(dyn_buffer_t *buf, uint32_t id, int64_t value) {
    if (!buf) return MKVTAG_ERR_INVALID_ARG;

    /* Determine minimum bytes needed for signed value */
    int data_size;
    if (value == 0) {
        data_size = 1;
    } else if (value >= -128 && value <= 127) {
        data_size = 1;
    } else if (value >= -32768 && value <= 32767) {
        data_size = 2;
    } else if (value >= -8388608 && value <= 8388607) {
        data_size = 3;
    } else if (value >= -2147483648LL && value <= 2147483647LL) {
        data_size = 4;
    } else {
        data_size = 8;
    }

    int err = write_id(buf, id);
    if (err != MKVTAG_OK) return err;

    err = write_size(buf, data_size);
    if (err != MKVTAG_OK) return err;

    /* Write value in big-endian */
    uint8_t data[8];
    uint64_t uval = (uint64_t)value;
    for (int i = data_size - 1; i >= 0; i--) {
        data[i] = (uint8_t)(uval & 0xFF);
        uval >>= 8;
    }

    return buffer_append(buf, data, data_size);
}

int ebml_write_string_element(dyn_buffer_t *buf, uint32_t id, const char *str) {
    if (!buf || !str) return MKVTAG_ERR_INVALID_ARG;

    size_t str_len = strlen(str);

    int err = write_id(buf, id);
    if (err != MKVTAG_OK) return err;

    err = write_size(buf, str_len);
    if (err != MKVTAG_OK) return err;

    return buffer_append(buf, str, str_len);
}

int ebml_write_binary_element(dyn_buffer_t *buf, uint32_t id,
                              const uint8_t *data, size_t size) {
    if (!buf) return MKVTAG_ERR_INVALID_ARG;

    int err = write_id(buf, id);
    if (err != MKVTAG_OK) return err;

    err = write_size(buf, size);
    if (err != MKVTAG_OK) return err;

    if (size > 0 && data) {
        return buffer_append(buf, data, size);
    }

    return MKVTAG_OK;
}

int ebml_write_void_element(dyn_buffer_t *buf, uint64_t total_size) {
    if (!buf) return MKVTAG_ERR_INVALID_ARG;

    if (total_size < 2) {
        /* Minimum Void element is 2 bytes: 1 byte ID + 1 byte size(0) */
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Void element ID is 1 byte: 0xEC */
    int err = buffer_append_byte(buf, EBML_ID_VOID);
    if (err != MKVTAG_OK) return err;

    /* Calculate content size: total - 1 (ID) - size_vint_length */
    uint64_t content_size;

    /* We need to figure out how many bytes the size VINT will take */
    /* Try increasing VINT sizes until it works */
    for (int vint_len = 1; vint_len <= 8; vint_len++) {
        content_size = total_size - 1 - vint_len;

        int needed = ebml_vint_size(content_size);
        if (needed == vint_len) {
            /* Write size with this length */
            uint8_t size_bytes[8];
            err = ebml_vint_encode_fixed(content_size, vint_len, size_bytes);
            if (err != MKVTAG_OK) return err;

            err = buffer_append(buf, size_bytes, vint_len);
            if (err != MKVTAG_OK) return err;

            /* Write zero padding */
            uint8_t zero = 0;
            for (uint64_t i = 0; i < content_size; i++) {
                err = buffer_append_byte(buf, zero);
                if (err != MKVTAG_OK) return err;
            }

            return MKVTAG_OK;
        }

        /* If the needed vint length is less than our current try,
         * we can use a longer encoding to make it fit */
        if (needed < vint_len) {
            content_size = total_size - 1 - vint_len;
            uint8_t size_bytes[8];
            err = ebml_vint_encode_fixed(content_size, vint_len, size_bytes);
            if (err != MKVTAG_OK) return err;

            err = buffer_append(buf, size_bytes, vint_len);
            if (err != MKVTAG_OK) return err;

            uint8_t zero = 0;
            for (uint64_t i = 0; i < content_size; i++) {
                err = buffer_append_byte(buf, zero);
                if (err != MKVTAG_OK) return err;
            }

            return MKVTAG_OK;
        }
    }

    return MKVTAG_ERR_VINT_OVERFLOW;
}

size_t ebml_uint_element_size(uint32_t id, uint64_t value) {
    int data_size = uint_data_size(value);
    if (data_size == 0) data_size = 1;
    return ebml_id_size(id) + ebml_vint_size(data_size) + data_size;
}

size_t ebml_string_element_size(uint32_t id, const char *str) {
    size_t str_len = str ? strlen(str) : 0;
    return ebml_id_size(id) + ebml_vint_size(str_len) + str_len;
}

size_t ebml_binary_element_size(uint32_t id, size_t data_size) {
    return ebml_id_size(id) + ebml_vint_size(data_size) + data_size;
}

size_t ebml_master_header_size(uint32_t id, uint64_t content_size) {
    return ebml_id_size(id) + ebml_vint_size(content_size);
}
