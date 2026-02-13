/*
 * ebml_reader.h - Stream-based EBML element reading
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef EBML_READER_H
#define EBML_READER_H

#include <stddef.h>
#include <stdint.h>
#include <tag_common/file_io.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * EBML element header information.
 */
typedef struct ebml_element {
    uint32_t id;            /* Element ID */
    uint64_t size;          /* Content size (may be EBML_SIZE_UNKNOWN) */
    int64_t data_offset;    /* File offset of element content */
    int64_t end_offset;     /* File offset of element end (data_offset + size) */
    int size_length;        /* Number of bytes used to encode size */
    int id_length;          /* Number of bytes used to encode ID */
    int is_unknown_size;    /* Whether size is unknown/unbounded */
} ebml_element_t;

/* Marker for unknown/unbounded element size */
#define EBML_SIZE_UNKNOWN UINT64_MAX

/**
 * Read an element header from the file.
 *
 * After this call, the file position is at the start of the element content.
 *
 * @param handle The file handle
 * @param element Output: element information
 * @return 0 on success, negative error code on failure
 */
int ebml_read_element_header(file_handle_t *handle, ebml_element_t *element);

/**
 * Peek at an element header without advancing the file position.
 *
 * @param handle The file handle
 * @param element Output: element information
 * @return 0 on success, negative error code on failure
 */
int ebml_peek_element_header(file_handle_t *handle, ebml_element_t *element);

/**
 * Skip to the end of an element's content.
 *
 * @param handle The file handle
 * @param element The element to skip
 * @return 0 on success, negative error code on failure
 */
int ebml_skip_element(file_handle_t *handle, const ebml_element_t *element);

/**
 * Read an element's content as an unsigned integer.
 *
 * @param handle The file handle
 * @param element The element to read
 * @param value Output: the integer value
 * @return 0 on success, negative error code on failure
 */
int ebml_read_uint(file_handle_t *handle, const ebml_element_t *element,
                   uint64_t *value);

/**
 * Read an element's content as a signed integer.
 *
 * @param handle The file handle
 * @param element The element to read
 * @param value Output: the integer value
 * @return 0 on success, negative error code on failure
 */
int ebml_read_int(file_handle_t *handle, const ebml_element_t *element,
                  int64_t *value);

/**
 * Read an element's content as a float.
 *
 * @param handle The file handle
 * @param element The element to read (must be 4 or 8 bytes)
 * @param value Output: the float value
 * @return 0 on success, negative error code on failure
 */
int ebml_read_float(file_handle_t *handle, const ebml_element_t *element,
                    double *value);

/**
 * Read an element's content as a UTF-8 string.
 *
 * @param handle The file handle
 * @param element The element to read
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @return 0 on success, negative error code on failure
 *
 * The string is null-terminated. If the buffer is too small,
 * MKVTAG_ERR_TAG_TOO_LARGE is returned.
 */
int ebml_read_string(file_handle_t *handle, const ebml_element_t *element,
                     char *buffer, size_t buffer_size);

/**
 * Read an element's content as a dynamically allocated UTF-8 string.
 *
 * @param handle The file handle
 * @param element The element to read
 * @param string Output: pointer to allocated string (caller must free)
 * @return 0 on success, negative error code on failure
 */
int ebml_read_string_alloc(file_handle_t *handle, const ebml_element_t *element,
                           char **string);

/**
 * Read an element's content as binary data.
 *
 * @param handle The file handle
 * @param element The element to read
 * @param buffer Output buffer
 * @param buffer_size Size of output buffer
 * @param bytes_read Output: number of bytes read
 * @return 0 on success, negative error code on failure
 */
int ebml_read_binary(file_handle_t *handle, const ebml_element_t *element,
                     uint8_t *buffer, size_t buffer_size, size_t *bytes_read);

/**
 * Read an element's content as dynamically allocated binary data.
 *
 * @param handle The file handle
 * @param element The element to read
 * @param data Output: pointer to allocated data (caller must free)
 * @param size Output: size of the data
 * @return 0 on success, negative error code on failure
 */
int ebml_read_binary_alloc(file_handle_t *handle, const ebml_element_t *element,
                           uint8_t **data, size_t *size);

/**
 * Check if we're at or past the end of a master element.
 *
 * @param handle The file handle
 * @param parent The parent master element
 * @return Non-zero if at/past end, 0 if more children remain
 */
int ebml_at_element_end(file_handle_t *handle, const ebml_element_t *parent);

#ifdef __cplusplus
}
#endif

#endif /* EBML_READER_H */
