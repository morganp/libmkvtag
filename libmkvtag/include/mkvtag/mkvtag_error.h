/*
 * mkvtag_error.h - Error codes for libmkvtag
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef MKVTAG_ERROR_H
#define MKVTAG_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

/* Success */
#define MKVTAG_OK                    0

/* General errors */
#define MKVTAG_ERR_INVALID_ARG      -1   /* Invalid argument */
#define MKVTAG_ERR_NO_MEMORY        -2   /* Memory allocation failed */
#define MKVTAG_ERR_IO               -3   /* I/O error */
#define MKVTAG_ERR_NOT_OPEN         -4   /* File not open */
#define MKVTAG_ERR_ALREADY_OPEN     -5   /* File already open */
#define MKVTAG_ERR_READ_ONLY        -6   /* File opened read-only */

/* Format errors */
#define MKVTAG_ERR_NOT_EBML         -10  /* Not a valid EBML file */
#define MKVTAG_ERR_NOT_MKV          -11  /* Not a Matroska file */
#define MKVTAG_ERR_CORRUPT          -12  /* File is corrupted */
#define MKVTAG_ERR_TRUNCATED        -13  /* Unexpected end of file */
#define MKVTAG_ERR_INVALID_VINT     -14  /* Invalid VINT encoding */
#define MKVTAG_ERR_VINT_OVERFLOW    -15  /* VINT value too large */

/* Tag errors */
#define MKVTAG_ERR_NO_TAGS          -20  /* No Tags element found */
#define MKVTAG_ERR_TAG_NOT_FOUND    -21  /* Specific tag not found */
#define MKVTAG_ERR_TAG_TOO_LARGE    -22  /* Tag data too large for buffer */

/* Write errors */
#define MKVTAG_ERR_NO_SPACE         -30  /* Not enough space to write */
#define MKVTAG_ERR_WRITE_FAILED     -31  /* Write operation failed */
#define MKVTAG_ERR_SEEK_FAILED      -32  /* Seek operation failed */

/**
 * Get a human-readable error message for an error code.
 *
 * @param error The error code
 * @return A static string describing the error
 */
const char *mkvtag_strerror(int error);

#ifdef __cplusplus
}
#endif

#endif /* MKVTAG_ERROR_H */
