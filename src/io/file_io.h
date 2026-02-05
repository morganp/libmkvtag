/*
 * file_io.h - Buffered file I/O interface
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef FILE_IO_H
#define FILE_IO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default read buffer size (8KB) */
#define FILE_IO_BUFFER_SIZE 8192

/**
 * Opaque file handle structure.
 */
typedef struct file_handle file_handle_t;

/**
 * Open a file for reading.
 *
 * @param path Path to the file (UTF-8)
 * @param handle Output: pointer to receive the file handle
 * @return 0 on success, negative error code on failure
 */
int file_open_read(const char *path, file_handle_t **handle);

/**
 * Open a file for reading and writing.
 *
 * @param path Path to the file (UTF-8)
 * @param handle Output: pointer to receive the file handle
 * @return 0 on success, negative error code on failure
 */
int file_open_rw(const char *path, file_handle_t **handle);

/**
 * Close a file handle and free resources.
 *
 * @param handle The file handle to close (may be NULL)
 */
void file_close(file_handle_t *handle);

/**
 * Get the current file position.
 *
 * @param handle The file handle
 * @return Current position, or -1 on error
 */
int64_t file_tell(file_handle_t *handle);

/**
 * Seek to a position in the file.
 *
 * @param handle The file handle
 * @param offset The offset to seek to
 * @param whence SEEK_SET, SEEK_CUR, or SEEK_END
 * @return 0 on success, negative error code on failure
 */
int file_seek(file_handle_t *handle, int64_t offset, int whence);

/**
 * Get the total file size.
 *
 * @param handle The file handle
 * @return File size in bytes, or -1 on error
 */
int64_t file_size(file_handle_t *handle);

/**
 * Read data from the file.
 *
 * @param handle The file handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @param bytes_read Output: number of bytes actually read
 * @return 0 on success, negative error code on failure
 */
int file_read(file_handle_t *handle, void *buffer, size_t size,
              size_t *bytes_read);

/**
 * Read exactly `size` bytes from the file.
 * Returns an error if fewer bytes are available.
 *
 * @param handle The file handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to read
 * @return 0 on success, negative error code on failure
 */
int file_read_exact(file_handle_t *handle, void *buffer, size_t size);

/**
 * Peek at data without advancing the file position.
 *
 * @param handle The file handle
 * @param buffer Buffer to read into
 * @param size Number of bytes to peek
 * @param bytes_read Output: number of bytes actually read
 * @return 0 on success, negative error code on failure
 */
int file_peek(file_handle_t *handle, void *buffer, size_t size,
              size_t *bytes_read);

/**
 * Read a single byte from the file.
 *
 * @param handle The file handle
 * @param byte Output: the byte read
 * @return 0 on success, negative error code on failure
 */
int file_read_byte(file_handle_t *handle, uint8_t *byte);

/**
 * Write data to the file.
 *
 * @param handle The file handle
 * @param buffer Buffer containing data to write
 * @param size Number of bytes to write
 * @return 0 on success, negative error code on failure
 */
int file_write(file_handle_t *handle, const void *buffer, size_t size);

/**
 * Flush any buffered writes to disk.
 *
 * @param handle The file handle
 * @return 0 on success, negative error code on failure
 */
int file_flush(file_handle_t *handle);

/**
 * Check if the file is open for writing.
 *
 * @param handle The file handle
 * @return Non-zero if writable, 0 if read-only
 */
int file_is_writable(file_handle_t *handle);

/**
 * Skip forward in the file without reading data.
 *
 * @param handle The file handle
 * @param bytes Number of bytes to skip
 * @return 0 on success, negative error code on failure
 */
int file_skip(file_handle_t *handle, int64_t bytes);

#ifdef __cplusplus
}
#endif

#endif /* FILE_IO_H */
