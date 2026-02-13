/*
 * ebml_reader.c - Stream-based EBML element reading
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "ebml_reader.h"
#include "ebml_vint.h"
#include "../include/mkvtag/mkvtag_error.h"

#include <stdlib.h>
#include <string.h>

int ebml_read_element_header(file_handle_t *handle, ebml_element_t *element) {
    if (!handle || !element) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    int64_t start_pos = file_tell(handle);
    if (start_pos < 0) {
        return MKVTAG_ERR_IO;
    }

    /* Read element ID (up to 4 bytes for EBML/Matroska) */
    uint8_t buffer[8];
    int err = file_read(handle, buffer, 1);
    if (err != 0) {
        return MKVTAG_ERR_TRUNCATED;
    }

    int id_length = ebml_vint_length(buffer[0]);
    if (id_length == 0 || id_length > 4) {
        return MKVTAG_ERR_INVALID_VINT;
    }

    /* Read remaining ID bytes */
    if (id_length > 1) {
        err = file_read(handle, buffer + 1, id_length - 1);
        if (err != 0) {
            return MKVTAG_ERR_TRUNCATED;
        }
    }

    /* Decode element ID (includes VINT marker bits) */
    int dummy;
    err = ebml_id_decode(buffer, id_length, &element->id, &dummy);
    if (err != MKVTAG_OK) {
        return err;
    }
    element->id_length = id_length;

    /* Read element size VINT */
    err = file_read(handle, buffer, 1);
    if (err != 0) {
        return MKVTAG_ERR_TRUNCATED;
    }

    int size_length = ebml_vint_length(buffer[0]);
    if (size_length == 0 || size_length > 8) {
        return MKVTAG_ERR_INVALID_VINT;
    }

    /* Read remaining size bytes */
    if (size_length > 1) {
        err = file_read(handle, buffer + 1, size_length - 1);
        if (err != 0) {
            return MKVTAG_ERR_TRUNCATED;
        }
    }

    /* Decode size */
    uint64_t size;
    err = ebml_vint_decode(buffer, size_length, &size, &dummy);
    if (err != MKVTAG_OK) {
        return err;
    }

    element->size_length = size_length;
    element->is_unknown_size = ebml_vint_is_unknown(size, size_length);
    element->size = element->is_unknown_size ? EBML_SIZE_UNKNOWN : size;

    /* Calculate offsets */
    element->data_offset = file_tell(handle);
    if (element->is_unknown_size) {
        element->end_offset = file_size(handle);
    } else {
        element->end_offset = element->data_offset + (int64_t)element->size;
    }

    return MKVTAG_OK;
}

int ebml_peek_element_header(file_handle_t *handle, ebml_element_t *element) {
    if (!handle || !element) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    int64_t pos = file_tell(handle);
    int err = ebml_read_element_header(handle, element);
    file_seek(handle, pos);
    return err;
}

int ebml_skip_element(file_handle_t *handle, const ebml_element_t *element) {
    if (!handle || !element) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (element->is_unknown_size) {
        /* Can't skip unknown-size elements reliably */
        return MKVTAG_ERR_CORRUPT;
    }

    return file_seek(handle, element->end_offset);
}

int ebml_read_uint(file_handle_t *handle, const ebml_element_t *element,
                   uint64_t *value) {
    if (!handle || !element || !value) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (element->size > 8) {
        return MKVTAG_ERR_VINT_OVERFLOW;
    }

    /* Seek to element data */
    int err = file_seek(handle, element->data_offset);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Handle empty element (value is 0) */
    if (element->size == 0) {
        *value = 0;
        return MKVTAG_OK;
    }

    /* Read data in big-endian format */
    uint8_t buffer[8];
    err = file_read(handle, buffer, (size_t)element->size);
    if (err != MKVTAG_OK) {
        return err;
    }

    uint64_t result = 0;
    for (size_t i = 0; i < element->size; i++) {
        result = (result << 8) | buffer[i];
    }

    *value = result;
    return MKVTAG_OK;
}

int ebml_read_int(file_handle_t *handle, const ebml_element_t *element,
                  int64_t *value) {
    if (!handle || !element || !value) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (element->size > 8) {
        return MKVTAG_ERR_VINT_OVERFLOW;
    }

    /* Seek to element data */
    int err = file_seek(handle, element->data_offset);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Handle empty element (value is 0) */
    if (element->size == 0) {
        *value = 0;
        return MKVTAG_OK;
    }

    /* Read data in big-endian format */
    uint8_t buffer[8];
    err = file_read(handle, buffer, (size_t)element->size);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Sign-extend from the first byte */
    int64_t result = (buffer[0] & 0x80) ? -1 : 0;
    for (size_t i = 0; i < element->size; i++) {
        result = (result << 8) | buffer[i];
    }

    *value = result;
    return MKVTAG_OK;
}

int ebml_read_float(file_handle_t *handle, const ebml_element_t *element,
                    double *value) {
    if (!handle || !element || !value) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (element->size != 4 && element->size != 8 && element->size != 0) {
        return MKVTAG_ERR_CORRUPT;
    }

    /* Seek to element data */
    int err = file_seek(handle, element->data_offset);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Handle empty element (value is 0.0) */
    if (element->size == 0) {
        *value = 0.0;
        return MKVTAG_OK;
    }

    uint8_t buffer[8];
    err = file_read(handle, buffer, (size_t)element->size);
    if (err != MKVTAG_OK) {
        return err;
    }

    if (element->size == 4) {
        /* 32-bit float */
        uint32_t bits = ((uint32_t)buffer[0] << 24) |
                        ((uint32_t)buffer[1] << 16) |
                        ((uint32_t)buffer[2] << 8) |
                        buffer[3];
        float f;
        memcpy(&f, &bits, sizeof(f));
        *value = (double)f;
    } else {
        /* 64-bit double */
        uint64_t bits = ((uint64_t)buffer[0] << 56) |
                        ((uint64_t)buffer[1] << 48) |
                        ((uint64_t)buffer[2] << 40) |
                        ((uint64_t)buffer[3] << 32) |
                        ((uint64_t)buffer[4] << 24) |
                        ((uint64_t)buffer[5] << 16) |
                        ((uint64_t)buffer[6] << 8) |
                        buffer[7];
        memcpy(value, &bits, sizeof(*value));
    }

    return MKVTAG_OK;
}

int ebml_read_string(file_handle_t *handle, const ebml_element_t *element,
                     char *buffer, size_t buffer_size) {
    if (!handle || !element || !buffer || buffer_size == 0) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Check buffer size (need room for null terminator) */
    if (element->size >= buffer_size) {
        return MKVTAG_ERR_TAG_TOO_LARGE;
    }

    /* Seek to element data */
    int err = file_seek(handle, element->data_offset);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Handle empty string */
    if (element->size == 0) {
        buffer[0] = '\0';
        return MKVTAG_OK;
    }

    /* Read string data */
    err = file_read(handle, buffer, (size_t)element->size);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Null-terminate (and handle embedded nulls by truncating) */
    buffer[element->size] = '\0';

    /* EBML strings may have trailing nulls for padding - find actual length */
    size_t len = (size_t)element->size;
    while (len > 0 && buffer[len - 1] == '\0') {
        len--;
    }
    buffer[len] = '\0';

    return MKVTAG_OK;
}

int ebml_read_string_alloc(file_handle_t *handle, const ebml_element_t *element,
                           char **string) {
    if (!handle || !element || !string) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Allocate buffer */
    size_t alloc_size = (size_t)element->size + 1;
    char *buffer = malloc(alloc_size);
    if (!buffer) {
        return MKVTAG_ERR_NO_MEMORY;
    }

    /* Read string */
    int err = ebml_read_string(handle, element, buffer, alloc_size);
    if (err != MKVTAG_OK) {
        free(buffer);
        return err;
    }

    *string = buffer;
    return MKVTAG_OK;
}

int ebml_read_binary(file_handle_t *handle, const ebml_element_t *element,
                     uint8_t *buffer, size_t buffer_size, size_t *bytes_read) {
    if (!handle || !element || !buffer || !bytes_read) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Seek to element data */
    int err = file_seek(handle, element->data_offset);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Read as much as will fit */
    size_t to_read = (element->size < buffer_size) ? (size_t)element->size : buffer_size;
    err = file_read(handle, buffer, to_read);
    if (err != MKVTAG_OK) {
        return err;
    }

    *bytes_read = to_read;
    return MKVTAG_OK;
}

int ebml_read_binary_alloc(file_handle_t *handle, const ebml_element_t *element,
                           uint8_t **data, size_t *size) {
    if (!handle || !element || !data || !size) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (element->size == 0) {
        *data = NULL;
        *size = 0;
        return MKVTAG_OK;
    }

    /* Allocate buffer */
    uint8_t *buffer = malloc((size_t)element->size);
    if (!buffer) {
        return MKVTAG_ERR_NO_MEMORY;
    }

    /* Read data */
    size_t bytes_read;
    int err = ebml_read_binary(handle, element, buffer, (size_t)element->size, &bytes_read);
    if (err != MKVTAG_OK) {
        free(buffer);
        return err;
    }

    *data = buffer;
    *size = bytes_read;
    return MKVTAG_OK;
}

int ebml_at_element_end(file_handle_t *handle, const ebml_element_t *parent) {
    if (!handle || !parent) {
        return 1;
    }

    int64_t pos = file_tell(handle);
    return pos >= parent->end_offset;
}
