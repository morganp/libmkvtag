/*
 * ebml_writer.h - EBML element writing/serialization
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef EBML_WRITER_H
#define EBML_WRITER_H

#include <stddef.h>
#include <stdint.h>
#include <tag_common/buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Write a master element header (ID + size) to a buffer.
 *
 * @param buf Output buffer
 * @param id Element ID
 * @param content_size Size of the content that will follow
 * @return 0 on success, negative error code on failure
 */
int ebml_write_master_element_header(dyn_buffer_t *buf, uint32_t id,
                                     uint64_t content_size);

/**
 * Write a complete unsigned integer element (ID + size + data).
 *
 * @param buf Output buffer
 * @param id Element ID
 * @param value The unsigned integer value
 * @return 0 on success, negative error code on failure
 */
int ebml_write_uint_element(dyn_buffer_t *buf, uint32_t id, uint64_t value);

/**
 * Write a complete signed integer element (ID + size + data).
 *
 * @param buf Output buffer
 * @param id Element ID
 * @param value The signed integer value
 * @return 0 on success, negative error code on failure
 */
int ebml_write_int_element(dyn_buffer_t *buf, uint32_t id, int64_t value);

/**
 * Write a complete UTF-8 string element (ID + size + data).
 *
 * @param buf Output buffer
 * @param id Element ID
 * @param str The string value
 * @return 0 on success, negative error code on failure
 */
int ebml_write_string_element(dyn_buffer_t *buf, uint32_t id, const char *str);

/**
 * Write a complete binary element (ID + size + data).
 *
 * @param buf Output buffer
 * @param id Element ID
 * @param data The binary data
 * @param size Size of the binary data
 * @return 0 on success, negative error code on failure
 */
int ebml_write_binary_element(dyn_buffer_t *buf, uint32_t id,
                              const uint8_t *data, size_t size);

/**
 * Write a Void element of the specified total size.
 *
 * @param buf Output buffer
 * @param total_size Total size including ID and size VINT
 * @return 0 on success, negative error code on failure
 */
int ebml_write_void_element(dyn_buffer_t *buf, uint64_t total_size);

/**
 * Calculate the total byte size of a uint element.
 *
 * @param id Element ID
 * @param value The value
 * @return Total byte size
 */
size_t ebml_uint_element_size(uint32_t id, uint64_t value);

/**
 * Calculate the total byte size of a string element.
 *
 * @param id Element ID
 * @param str The string
 * @return Total byte size
 */
size_t ebml_string_element_size(uint32_t id, const char *str);

/**
 * Calculate the total byte size of a binary element.
 *
 * @param id Element ID
 * @param data_size Size of the binary data
 * @return Total byte size
 */
size_t ebml_binary_element_size(uint32_t id, size_t data_size);

/**
 * Calculate the total byte size of a master element header (ID + size VINT).
 *
 * @param id Element ID
 * @param content_size Size of the content
 * @return Total header byte size
 */
size_t ebml_master_header_size(uint32_t id, uint64_t content_size);

#ifdef __cplusplus
}
#endif

#endif /* EBML_WRITER_H */
