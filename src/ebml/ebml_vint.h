/*
 * ebml_vint.h - EBML variable-length integer encoding/decoding
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef EBML_VINT_H
#define EBML_VINT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum VINT value for each byte length */
#define EBML_VINT_MAX_1  ((1ULL << 7) - 2)   /* 126 */
#define EBML_VINT_MAX_2  ((1ULL << 14) - 2)  /* 16382 */
#define EBML_VINT_MAX_3  ((1ULL << 21) - 2)  /* 2097150 */
#define EBML_VINT_MAX_4  ((1ULL << 28) - 2)  /* 268435454 */
#define EBML_VINT_MAX_5  ((1ULL << 35) - 2)  /* 34359738366 */
#define EBML_VINT_MAX_6  ((1ULL << 42) - 2)  /* 4398046511102 */
#define EBML_VINT_MAX_7  ((1ULL << 49) - 2)  /* 562949953421310 */
#define EBML_VINT_MAX_8  ((1ULL << 56) - 2)  /* 72057594037927934 */

/* Unknown/reserved size marker (all 1s in data bits) */
#define EBML_VINT_UNKNOWN_1  0xFF
#define EBML_VINT_UNKNOWN_2  0x7FFF
#define EBML_VINT_UNKNOWN_3  0x3FFFFF
#define EBML_VINT_UNKNOWN_4  0x1FFFFFFF
#define EBML_VINT_UNKNOWN_5  0x0FFFFFFFFFULL
#define EBML_VINT_UNKNOWN_6  0x07FFFFFFFFFFULL
#define EBML_VINT_UNKNOWN_7  0x03FFFFFFFFFFFFULL
#define EBML_VINT_UNKNOWN_8  0x01FFFFFFFFFFFFFFULL

/**
 * Determine the number of bytes needed to encode a value as a VINT.
 *
 * @param value The value to encode
 * @return Number of bytes (1-8), or 0 if value is too large
 */
int ebml_vint_size(uint64_t value);

/**
 * Determine the number of bytes in a VINT by examining its first byte.
 *
 * @param first_byte The first byte of the VINT
 * @return Number of bytes (1-8), or 0 if invalid
 */
int ebml_vint_length(uint8_t first_byte);

/**
 * Decode a VINT from a buffer.
 *
 * @param data Pointer to the VINT data
 * @param size Available bytes in the buffer
 * @param value Output: the decoded value
 * @param bytes_read Output: number of bytes consumed
 * @return 0 on success, negative error code on failure
 */
int ebml_vint_decode(const uint8_t *data, size_t size,
                     uint64_t *value, int *bytes_read);

/**
 * Decode an element ID from a buffer.
 * Element IDs include the VINT marker bits in their value.
 *
 * @param data Pointer to the ID data
 * @param size Available bytes in the buffer
 * @param id Output: the decoded element ID
 * @param bytes_read Output: number of bytes consumed
 * @return 0 on success, negative error code on failure
 */
int ebml_id_decode(const uint8_t *data, size_t size,
                   uint32_t *id, int *bytes_read);

/**
 * Encode a value as a VINT into a buffer.
 *
 * @param value The value to encode
 * @param data Output buffer (must have at least 8 bytes)
 * @param bytes_written Output: number of bytes written
 * @return 0 on success, negative error code on failure
 */
int ebml_vint_encode(uint64_t value, uint8_t *data, int *bytes_written);

/**
 * Encode a value as a VINT with a specific length.
 * Useful for creating reserved space or matching existing element sizes.
 *
 * @param value The value to encode
 * @param length The desired length (1-8)
 * @param data Output buffer (must have at least `length` bytes)
 * @return 0 on success, negative error code on failure
 */
int ebml_vint_encode_fixed(uint64_t value, int length, uint8_t *data);

/**
 * Encode an element ID into a buffer.
 *
 * @param id The element ID to encode
 * @param data Output buffer (must have at least 4 bytes)
 * @param bytes_written Output: number of bytes written
 * @return 0 on success, negative error code on failure
 */
int ebml_id_encode(uint32_t id, uint8_t *data, int *bytes_written);

/**
 * Check if a VINT represents an unknown/unbounded size.
 *
 * @param value The decoded VINT value
 * @param length The number of bytes used to encode the VINT
 * @return Non-zero if the size is unknown, 0 otherwise
 */
int ebml_vint_is_unknown(uint64_t value, int length);

#ifdef __cplusplus
}
#endif

#endif /* EBML_VINT_H */
