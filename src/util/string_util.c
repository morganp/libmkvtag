/*
 * string_util.c - UTF-8 string utilities
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "string_util.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

char *str_dup(const char *str) {
    if (!str) {
        return NULL;
    }

    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    if (copy) {
        memcpy(copy, str, len + 1);
    }
    return copy;
}

int str_casecmp(const char *a, const char *b) {
    if (!a && !b) return 0;
    if (!a) return -1;
    if (!b) return 1;

    while (*a && *b) {
        int diff = tolower((unsigned char)*a) - tolower((unsigned char)*b);
        if (diff != 0) {
            return diff;
        }
        a++;
        b++;
    }

    return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

int str_copy(char *dst, const char *src, size_t dst_size) {
    if (!dst || dst_size == 0) {
        return -1;
    }

    if (!src) {
        dst[0] = '\0';
        return 0;
    }

    size_t src_len = strlen(src);
    if (src_len >= dst_size) {
        memcpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
        return -1;
    }

    memcpy(dst, src, src_len + 1);
    return (int)src_len;
}
