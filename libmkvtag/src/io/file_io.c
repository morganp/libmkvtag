/*
 * file_io.c - Buffered file I/O implementation using POSIX
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "file_io.h"
#include "../../include/mkvtag/mkvtag_error.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

struct file_handle {
    int fd;
    int writable;
    int64_t file_size;

    /* Read buffer */
    uint8_t buffer[FILE_IO_BUFFER_SIZE];
    size_t buffer_pos;      /* Current position within buffer */
    size_t buffer_len;      /* Valid bytes in buffer */
    int64_t buffer_offset;  /* File offset of buffer start */
};

int file_open_read(const char *path, file_handle_t **handle) {
    if (!path || !handle) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    file_handle_t *h = calloc(1, sizeof(file_handle_t));
    if (!h) {
        return MKVTAG_ERR_NO_MEMORY;
    }

    h->fd = open(path, O_RDONLY);
    if (h->fd < 0) {
        free(h);
        return MKVTAG_ERR_IO;
    }

    h->writable = 0;

    /* Get file size */
    struct stat st;
    if (fstat(h->fd, &st) < 0) {
        close(h->fd);
        free(h);
        return MKVTAG_ERR_IO;
    }
    h->file_size = st.st_size;

    /* Initialize buffer state */
    h->buffer_pos = 0;
    h->buffer_len = 0;
    h->buffer_offset = 0;

    *handle = h;
    return MKVTAG_OK;
}

int file_open_rw(const char *path, file_handle_t **handle) {
    if (!path || !handle) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    file_handle_t *h = calloc(1, sizeof(file_handle_t));
    if (!h) {
        return MKVTAG_ERR_NO_MEMORY;
    }

    h->fd = open(path, O_RDWR);
    if (h->fd < 0) {
        free(h);
        return MKVTAG_ERR_IO;
    }

    h->writable = 1;

    /* Get file size */
    struct stat st;
    if (fstat(h->fd, &st) < 0) {
        close(h->fd);
        free(h);
        return MKVTAG_ERR_IO;
    }
    h->file_size = st.st_size;

    /* Initialize buffer state */
    h->buffer_pos = 0;
    h->buffer_len = 0;
    h->buffer_offset = 0;

    *handle = h;
    return MKVTAG_OK;
}

void file_close(file_handle_t *handle) {
    if (handle) {
        if (handle->fd >= 0) {
            close(handle->fd);
        }
        free(handle);
    }
}

int64_t file_tell(file_handle_t *handle) {
    if (!handle) {
        return -1;
    }

    /* Current position is buffer start + position within buffer */
    return handle->buffer_offset + (int64_t)handle->buffer_pos;
}

int file_seek(file_handle_t *handle, int64_t offset, int whence) {
    if (!handle) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Calculate absolute target position */
    int64_t target;
    switch (whence) {
        case SEEK_SET:
            target = offset;
            break;
        case SEEK_CUR:
            target = file_tell(handle) + offset;
            break;
        case SEEK_END:
            target = handle->file_size + offset;
            break;
        default:
            return MKVTAG_ERR_INVALID_ARG;
    }

    if (target < 0) {
        return MKVTAG_ERR_SEEK_FAILED;
    }

    /* Check if target is within current buffer */
    int64_t buffer_end = handle->buffer_offset + (int64_t)handle->buffer_len;
    if (target >= handle->buffer_offset && target <= buffer_end) {
        handle->buffer_pos = (size_t)(target - handle->buffer_offset);
        return MKVTAG_OK;
    }

    /* Need to seek and invalidate buffer */
    off_t result = lseek(handle->fd, target, SEEK_SET);
    if (result < 0) {
        return MKVTAG_ERR_SEEK_FAILED;
    }

    /* Invalidate buffer */
    handle->buffer_offset = target;
    handle->buffer_pos = 0;
    handle->buffer_len = 0;

    return MKVTAG_OK;
}

int64_t file_size(file_handle_t *handle) {
    if (!handle) {
        return -1;
    }
    return handle->file_size;
}

/* Internal: refill the read buffer */
static int file_refill_buffer(file_handle_t *handle) {
    /* Update buffer offset to current file position */
    handle->buffer_offset = lseek(handle->fd, 0, SEEK_CUR);
    if (handle->buffer_offset < 0) {
        return MKVTAG_ERR_IO;
    }

    ssize_t n = read(handle->fd, handle->buffer, FILE_IO_BUFFER_SIZE);
    if (n < 0) {
        return MKVTAG_ERR_IO;
    }

    handle->buffer_pos = 0;
    handle->buffer_len = (size_t)n;
    return MKVTAG_OK;
}

int file_read(file_handle_t *handle, void *buffer, size_t size,
              size_t *bytes_read) {
    if (!handle || !buffer || !bytes_read) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    uint8_t *dst = buffer;
    size_t total_read = 0;

    while (size > 0) {
        /* Check if we need to refill the buffer */
        if (handle->buffer_pos >= handle->buffer_len) {
            int err = file_refill_buffer(handle);
            if (err != MKVTAG_OK) {
                *bytes_read = total_read;
                return err;
            }
            if (handle->buffer_len == 0) {
                /* EOF */
                break;
            }
        }

        /* Copy from buffer */
        size_t available = handle->buffer_len - handle->buffer_pos;
        size_t to_copy = (size < available) ? size : available;

        memcpy(dst, handle->buffer + handle->buffer_pos, to_copy);
        handle->buffer_pos += to_copy;
        dst += to_copy;
        size -= to_copy;
        total_read += to_copy;
    }

    *bytes_read = total_read;
    return MKVTAG_OK;
}

int file_read_exact(file_handle_t *handle, void *buffer, size_t size) {
    size_t bytes_read;
    int err = file_read(handle, buffer, size, &bytes_read);
    if (err != MKVTAG_OK) {
        return err;
    }
    if (bytes_read != size) {
        return MKVTAG_ERR_TRUNCATED;
    }
    return MKVTAG_OK;
}

int file_peek(file_handle_t *handle, void *buffer, size_t size,
              size_t *bytes_read) {
    if (!handle || !buffer || !bytes_read) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Save current position */
    int64_t pos = file_tell(handle);

    /* Read the data */
    int err = file_read(handle, buffer, size, bytes_read);

    /* Restore position */
    file_seek(handle, pos, SEEK_SET);

    return err;
}

int file_read_byte(file_handle_t *handle, uint8_t *byte) {
    if (!handle || !byte) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Check if we need to refill the buffer */
    if (handle->buffer_pos >= handle->buffer_len) {
        int err = file_refill_buffer(handle);
        if (err != MKVTAG_OK) {
            return err;
        }
        if (handle->buffer_len == 0) {
            return MKVTAG_ERR_TRUNCATED;
        }
    }

    *byte = handle->buffer[handle->buffer_pos++];
    return MKVTAG_OK;
}

int file_write(file_handle_t *handle, const void *buffer, size_t size) {
    if (!handle || !buffer) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (!handle->writable) {
        return MKVTAG_ERR_READ_ONLY;
    }

    /* Flush read buffer position to actual file position before writing */
    int64_t current_pos = file_tell(handle);
    if (lseek(handle->fd, current_pos, SEEK_SET) < 0) {
        return MKVTAG_ERR_SEEK_FAILED;
    }

    /* Write data */
    const uint8_t *src = buffer;
    size_t remaining = size;
    while (remaining > 0) {
        ssize_t n = write(handle->fd, src, remaining);
        if (n < 0) {
            return MKVTAG_ERR_WRITE_FAILED;
        }
        src += n;
        remaining -= n;
    }

    /* Invalidate read buffer since file content may have changed */
    handle->buffer_offset = current_pos + size;
    handle->buffer_pos = 0;
    handle->buffer_len = 0;

    /* Update file size if we wrote past the end */
    int64_t new_pos = current_pos + (int64_t)size;
    if (new_pos > handle->file_size) {
        handle->file_size = new_pos;
    }

    return MKVTAG_OK;
}

int file_flush(file_handle_t *handle) {
    if (!handle) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (handle->writable) {
        if (fsync(handle->fd) < 0) {
            return MKVTAG_ERR_IO;
        }
    }

    return MKVTAG_OK;
}

int file_is_writable(file_handle_t *handle) {
    if (!handle) {
        return 0;
    }
    return handle->writable;
}

int file_skip(file_handle_t *handle, int64_t bytes) {
    return file_seek(handle, bytes, SEEK_CUR);
}
