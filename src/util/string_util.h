/*
 * string_util.h - UTF-8 string utilities
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Duplicate a string.
 *
 * @param str The string to duplicate
 * @return New allocated copy, or NULL on failure
 */
char *str_dup(const char *str);

/**
 * Case-insensitive string comparison (ASCII).
 *
 * @param a First string
 * @param b Second string
 * @return 0 if equal, non-zero otherwise
 */
int str_casecmp(const char *a, const char *b);

/**
 * Safe string copy with null termination guarantee.
 *
 * @param dst Destination buffer
 * @param src Source string
 * @param dst_size Size of destination buffer
 * @return Number of characters copied (excluding null), or -1 if truncated
 */
int str_copy(char *dst, const char *src, size_t dst_size);

#ifdef __cplusplus
}
#endif

#endif /* STRING_UTIL_H */
