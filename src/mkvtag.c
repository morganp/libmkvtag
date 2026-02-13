/*
 * mkvtag.c - Main API implementation for libmkvtag
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "../include/mkvtag/mkvtag.h"
#include <tag_common/file_io.h>
#include <tag_common/buffer.h>
#include <tag_common/string_util.h>
#include "ebml/ebml_ids.h"
#include "ebml/ebml_reader.h"
#include "ebml/ebml_writer.h"
#include "ebml/ebml_vint.h"
#include "mkv/mkv_parser.h"
#include "mkv/mkv_tags.h"
#include "mkv/mkv_seekhead.h"

#include <stdlib.h>
#include <string.h>

/*
 * Internal context structure
 */
struct mkvtag_context {
    mkvtag_allocator_t allocator;
    int has_custom_allocator;

    file_handle_t *file;
    mkv_file_t mkv;
    int is_open;
    int is_writable;

    /* Cached tag collection (from last read) */
    mkvtag_collection_t *cached_tags;
};

/*
 * Allocator helpers
 */
static void ctx_free(mkvtag_context_t *ctx, void *ptr) {
    if (ctx && ctx->has_custom_allocator && ctx->allocator.free) {
        ctx->allocator.free(ptr, ctx->allocator.user_data);
        return;
    }
    free(ptr);
}

/*
 * Version
 */
const char *mkvtag_version(void) {
    return "1.0.0";
}

/*
 * Error messages
 */
const char *mkvtag_strerror(int error) {
    switch (error) {
        case MKVTAG_OK:                 return "Success";
        case MKVTAG_ERR_INVALID_ARG:    return "Invalid argument";
        case MKVTAG_ERR_NO_MEMORY:      return "Out of memory";
        case MKVTAG_ERR_IO:             return "I/O error";
        case MKVTAG_ERR_NOT_OPEN:       return "File not open";
        case MKVTAG_ERR_ALREADY_OPEN:   return "File already open";
        case MKVTAG_ERR_READ_ONLY:      return "File is read-only";
        case MKVTAG_ERR_NOT_EBML:       return "Not a valid EBML file";
        case MKVTAG_ERR_NOT_MKV:        return "Not a Matroska file";
        case MKVTAG_ERR_CORRUPT:        return "File is corrupted";
        case MKVTAG_ERR_TRUNCATED:      return "Unexpected end of file";
        case MKVTAG_ERR_INVALID_VINT:   return "Invalid VINT encoding";
        case MKVTAG_ERR_VINT_OVERFLOW:  return "VINT value too large";
        case MKVTAG_ERR_NO_TAGS:        return "No Tags element found";
        case MKVTAG_ERR_TAG_NOT_FOUND:  return "Tag not found";
        case MKVTAG_ERR_TAG_TOO_LARGE:  return "Tag data too large for buffer";
        case MKVTAG_ERR_NO_SPACE:       return "Not enough space to write tags";
        case MKVTAG_ERR_WRITE_FAILED:   return "Write operation failed";
        case MKVTAG_ERR_SEEK_FAILED:    return "Seek operation failed";
        default:                         return "Unknown error";
    }
}

/*
 * Context Management
 */

mkvtag_context_t *mkvtag_create(const mkvtag_allocator_t *allocator) {
    mkvtag_context_t *ctx;

    if (allocator && allocator->alloc) {
        ctx = allocator->alloc(sizeof(mkvtag_context_t), allocator->user_data);
    } else {
        ctx = calloc(1, sizeof(mkvtag_context_t));
    }

    if (!ctx) return NULL;

    memset(ctx, 0, sizeof(*ctx));

    if (allocator) {
        ctx->allocator = *allocator;
        ctx->has_custom_allocator = 1;
    }

    mkv_file_init(&ctx->mkv);

    return ctx;
}

void mkvtag_destroy(mkvtag_context_t *ctx) {
    if (!ctx) return;

    mkvtag_close(ctx);

    if (ctx->cached_tags) {
        mkv_tags_free_collection(ctx->cached_tags);
        ctx->cached_tags = NULL;
    }

    ctx_free(ctx, ctx);
}

/*
 * File Operations
 */

static int mkvtag_open_internal(mkvtag_context_t *ctx, const char *path, int writable) {
    if (!ctx || !path) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (ctx->is_open) {
        return MKVTAG_ERR_ALREADY_OPEN;
    }

    /* Open file */
    if (writable) {
        ctx->file = file_open_rw(path);
    } else {
        ctx->file = file_open_read(path);
    }
    if (!ctx->file) {
        return MKVTAG_ERR_IO;
    }

    /* Reset MKV state */
    mkv_file_init(&ctx->mkv);

    /* Parse EBML header */
    int err = mkv_parse_header(&ctx->mkv, ctx->file);
    if (err != MKVTAG_OK) {
        file_close(ctx->file);
        ctx->file = NULL;
        return err;
    }

    /* Parse file structure */
    err = mkv_parse_structure(&ctx->mkv);
    if (err != MKVTAG_OK) {
        file_close(ctx->file);
        ctx->file = NULL;
        return err;
    }

    ctx->is_open = 1;
    ctx->is_writable = writable;

    return MKVTAG_OK;
}

int mkvtag_open(mkvtag_context_t *ctx, const char *path) {
    return mkvtag_open_internal(ctx, path, 0);
}

int mkvtag_open_rw(mkvtag_context_t *ctx, const char *path) {
    return mkvtag_open_internal(ctx, path, 1);
}

void mkvtag_close(mkvtag_context_t *ctx) {
    if (!ctx) return;

    if (ctx->cached_tags) {
        mkv_tags_free_collection(ctx->cached_tags);
        ctx->cached_tags = NULL;
    }

    if (ctx->file) {
        file_close(ctx->file);
        ctx->file = NULL;
    }

    ctx->is_open = 0;
    ctx->is_writable = 0;
}

int mkvtag_is_open(const mkvtag_context_t *ctx) {
    if (!ctx) return 0;
    return ctx->is_open;
}

/*
 * Tag Reading
 */

int mkvtag_read_tags(mkvtag_context_t *ctx, mkvtag_collection_t **tags) {
    if (!ctx || !tags) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (!ctx->is_open) {
        return MKVTAG_ERR_NOT_OPEN;
    }

    /* Return cached tags if available */
    if (ctx->cached_tags) {
        *tags = ctx->cached_tags;
        return MKVTAG_OK;
    }

    /* Find Tags element */
    if (ctx->mkv.tags_offset < 0) {
        return MKVTAG_ERR_NO_TAGS;
    }

    /* Seek to Tags element */
    int err = file_seek(ctx->file, ctx->mkv.tags_offset);
    if (err != MKVTAG_OK) return err;

    /* Read Tags element header */
    ebml_element_t tags_element;
    err = ebml_read_element_header(ctx->file, &tags_element);
    if (err != MKVTAG_OK) return err;

    if (tags_element.id != MKV_ID_TAGS) {
        return MKVTAG_ERR_CORRUPT;
    }

    /* Parse tags */
    mkvtag_collection_t *collection;
    err = mkv_tags_parse(&ctx->mkv, &tags_element, &collection);
    if (err != MKVTAG_OK) return err;

    ctx->cached_tags = collection;
    *tags = collection;

    return MKVTAG_OK;
}

int mkvtag_read_tag_string(mkvtag_context_t *ctx, const char *name,
                           char *value, size_t size) {
    if (!ctx || !name || !value || size == 0) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Read all tags first */
    mkvtag_collection_t *tags;
    int err = mkvtag_read_tags(ctx, &tags);
    if (err != MKVTAG_OK) return err;

    /* Search for the tag */
    const mkvtag_tag_t *tag = tags->tags;
    while (tag) {
        /* Check all simple tags in each tag */
        const mkvtag_simple_tag_t *stag = tag->simple_tags;
        while (stag) {
            if (stag->name && str_casecmp(stag->name, name) == 0) {
                if (stag->value) {
                    if (str_copy(value, size, stag->value) < 0) {
                        return MKVTAG_ERR_TAG_TOO_LARGE;
                    }
                    return MKVTAG_OK;
                }
            }
            stag = stag->next;
        }
        tag = tag->next;
    }

    return MKVTAG_ERR_TAG_NOT_FOUND;
}

/*
 * Tag Writing
 */

/**
 * Write a Tags element at the given offset and fill remaining space with Void.
 */
static int write_tags_at(mkvtag_context_t *ctx, int64_t offset,
                         uint64_t available_space,
                         const mkvtag_collection_t *tags) {
    /* Serialize tag content */
    dyn_buffer_t content;
    buffer_init(&content);

    int err = mkv_tags_serialize(tags, &content);
    if (err != MKVTAG_OK) {
        buffer_free(&content);
        return err;
    }

    /* Build complete Tags element */
    dyn_buffer_t tags_buf;
    buffer_init(&tags_buf);

    err = ebml_write_master_element_header(&tags_buf, MKV_ID_TAGS, content.size);
    if (err != MKVTAG_OK) {
        buffer_free(&content);
        buffer_free(&tags_buf);
        return err;
    }

    err = buffer_append(&tags_buf, content.data, content.size);
    buffer_free(&content);
    if (err != MKVTAG_OK) {
        buffer_free(&tags_buf);
        return err;
    }

    /* Check if it fits */
    if (tags_buf.size > available_space) {
        buffer_free(&tags_buf);
        return MKVTAG_ERR_NO_SPACE;
    }

    /* Write Tags element */
    err = file_seek(ctx->file, offset);
    if (err != MKVTAG_OK) {
        buffer_free(&tags_buf);
        return err;
    }

    err = file_write(ctx->file, tags_buf.data, tags_buf.size);
    if (err != MKVTAG_OK) {
        buffer_free(&tags_buf);
        return err;
    }

    /* Fill remaining space with Void element */
    uint64_t remaining = available_space - tags_buf.size;
    buffer_free(&tags_buf);

    if (remaining >= 2) {
        dyn_buffer_t void_buf;
        buffer_init(&void_buf);

        err = ebml_write_void_element(&void_buf, remaining);
        if (err != MKVTAG_OK) {
            buffer_free(&void_buf);
            return err;
        }

        err = file_write(ctx->file, void_buf.data, void_buf.size);
        buffer_free(&void_buf);
        if (err != MKVTAG_OK) return err;
    } else if (remaining == 1) {
        /* Can't write a 1-byte Void element; just write a zero byte */
        uint8_t zero = 0;
        err = file_write(ctx->file, &zero, 1);
        if (err != MKVTAG_OK) return err;
    }

    return MKVTAG_OK;
}

/**
 * Try to replace existing Tags + following Void with new Tags.
 */
static int try_replace_existing_tags(mkvtag_context_t *ctx,
                                     const mkvtag_collection_t *tags) {
    if (ctx->mkv.tags_offset < 0) {
        return MKVTAG_ERR_NO_TAGS;
    }

    /* Read existing Tags element to find its total size */
    int err = file_seek(ctx->file, ctx->mkv.tags_offset);
    if (err != MKVTAG_OK) return err;

    ebml_element_t existing_tags;
    err = ebml_read_element_header(ctx->file, &existing_tags);
    if (err != MKVTAG_OK) return err;

    int64_t existing_end = existing_tags.end_offset;
    uint64_t available = (uint64_t)(existing_end - ctx->mkv.tags_offset);

    /* Check if there's a Void element right after the existing Tags */
    err = file_seek(ctx->file, existing_end);
    if (err == MKVTAG_OK) {
        ebml_element_t next_elem;
        err = ebml_peek_element_header(ctx->file, &next_elem);
        if (err == MKVTAG_OK && next_elem.id == EBML_ID_VOID) {
            /* Include the Void in our available space */
            available += (uint64_t)(next_elem.end_offset - existing_end);
        }
    }

    /* Try to write at existing location */
    err = write_tags_at(ctx, ctx->mkv.tags_offset, available, tags);
    if (err == MKVTAG_OK) {
        /* Update SeekHead if needed */
        mkv_seekhead_update_tags(&ctx->mkv, ctx->mkv.tags_offset);
        return MKVTAG_OK;
    }

    return err;
}

/**
 * Try to write Tags into the largest Void element.
 */
static int try_replace_void(mkvtag_context_t *ctx,
                            const mkvtag_collection_t *tags) {
    if (ctx->mkv.void_offset < 0 || ctx->mkv.void_size == 0) {
        return MKVTAG_ERR_NO_SPACE;
    }

    /* Don't use the Void that might be right after existing Tags
     * (that's handled by try_replace_existing_tags) */

    int err = write_tags_at(ctx, ctx->mkv.void_offset,
                            ctx->mkv.void_size, tags);
    if (err == MKVTAG_OK) {
        ctx->mkv.tags_offset = ctx->mkv.void_offset;
        mkv_seekhead_update_tags(&ctx->mkv, ctx->mkv.tags_offset);
        return MKVTAG_OK;
    }

    return err;
}

/**
 * Append Tags at the end of the Segment.
 */
static int try_append_tags(mkvtag_context_t *ctx,
                           const mkvtag_collection_t *tags) {
    /* Calculate where the Segment content ends */
    int64_t segment_content_end;
    if (ctx->mkv.segment_unknown_size) {
        segment_content_end = file_size(ctx->file);
    } else {
        segment_content_end = ctx->mkv.segment_data_offset +
                              (int64_t)ctx->mkv.segment_size;
    }

    /* Serialize tags */
    dyn_buffer_t content;
    buffer_init(&content);

    int err = mkv_tags_serialize(tags, &content);
    if (err != MKVTAG_OK) {
        buffer_free(&content);
        return err;
    }

    dyn_buffer_t tags_buf;
    buffer_init(&tags_buf);

    err = ebml_write_master_element_header(&tags_buf, MKV_ID_TAGS, content.size);
    if (err != MKVTAG_OK) {
        buffer_free(&content);
        buffer_free(&tags_buf);
        return err;
    }

    err = buffer_append(&tags_buf, content.data, content.size);
    buffer_free(&content);
    if (err != MKVTAG_OK) {
        buffer_free(&tags_buf);
        return err;
    }

    /* If Segment has a known size, we need to update it */
    if (!ctx->mkv.segment_unknown_size) {
        /* Update Segment size to include new Tags */
        uint64_t new_segment_size = ctx->mkv.segment_size + tags_buf.size;

        /* Re-encode Segment size (try to keep same VINT length) */
        int64_t size_offset = ctx->mkv.segment_offset +
                              ebml_id_size(MKV_ID_SEGMENT);

        /* Read current size encoding length */
        err = file_seek(ctx->file, size_offset);
        if (err != MKVTAG_OK) {
            buffer_free(&tags_buf);
            return err;
        }

        uint8_t first_byte;
        err = file_read(ctx->file, &first_byte, 1);
        if (err != MKVTAG_OK) {
            buffer_free(&tags_buf);
            return err;
        }

        int current_size_len = ebml_vint_length(first_byte);

        /* Try to encode new size with same length */
        uint8_t new_size_bytes[8];
        err = ebml_vint_encode_fixed(new_segment_size, current_size_len, new_size_bytes);
        if (err != MKVTAG_OK) {
            /* Can't fit new size in same number of bytes - use 8-byte encoding */
            buffer_free(&tags_buf);
            return MKVTAG_ERR_NO_SPACE;
        }

        /* Write updated Segment size */
        err = file_seek(ctx->file, size_offset);
        if (err != MKVTAG_OK) {
            buffer_free(&tags_buf);
            return err;
        }

        err = file_write(ctx->file, new_size_bytes, current_size_len);
        if (err != MKVTAG_OK) {
            buffer_free(&tags_buf);
            return err;
        }

        ctx->mkv.segment_size = new_segment_size;
    }

    /* Write Tags at end of Segment */
    err = file_seek(ctx->file, segment_content_end);
    if (err != MKVTAG_OK) {
        buffer_free(&tags_buf);
        return err;
    }

    int64_t new_tags_offset = segment_content_end;

    err = file_write(ctx->file, tags_buf.data, tags_buf.size);
    buffer_free(&tags_buf);
    if (err != MKVTAG_OK) return err;

    err = file_sync(ctx->file);
    if (err != MKVTAG_OK) return err;

    /* Void-out old Tags if they exist */
    if (ctx->mkv.tags_offset >= 0) {
        err = file_seek(ctx->file, ctx->mkv.tags_offset);
        if (err == MKVTAG_OK) {
            ebml_element_t old_tags;
            err = ebml_read_element_header(ctx->file, &old_tags);
            if (err == MKVTAG_OK) {
                uint64_t old_total = (uint64_t)(old_tags.end_offset - ctx->mkv.tags_offset);
                if (old_total >= 2) {
                    dyn_buffer_t void_buf;
                    buffer_init(&void_buf);
                    if (ebml_write_void_element(&void_buf, old_total) == MKVTAG_OK) {
                        file_seek(ctx->file, ctx->mkv.tags_offset);
                        file_write(ctx->file, void_buf.data, void_buf.size);
                    }
                    buffer_free(&void_buf);
                }
            }
        }
    }

    ctx->mkv.tags_offset = new_tags_offset;
    mkv_seekhead_update_tags(&ctx->mkv, new_tags_offset);

    return file_sync(ctx->file);
}

int mkvtag_write_tags(mkvtag_context_t *ctx, const mkvtag_collection_t *tags) {
    if (!ctx || !tags) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (!ctx->is_open) {
        return MKVTAG_ERR_NOT_OPEN;
    }

    if (!ctx->is_writable) {
        return MKVTAG_ERR_READ_ONLY;
    }

    /* Invalidate cached tags */
    if (ctx->cached_tags) {
        mkv_tags_free_collection(ctx->cached_tags);
        ctx->cached_tags = NULL;
    }

    /* Strategy 1: Replace existing Tags element (+ adjacent Void) */
    if (ctx->mkv.tags_offset >= 0) {
        int err = try_replace_existing_tags(ctx, tags);
        if (err == MKVTAG_OK) {
            return file_sync(ctx->file);
        }
    }

    /* Strategy 2: Replace Void element with Tags */
    {
        int err = try_replace_void(ctx, tags);
        if (err == MKVTAG_OK) {
            return file_sync(ctx->file);
        }
    }

    /* Strategy 3: Append at end of Segment */
    return try_append_tags(ctx, tags);
}

int mkvtag_set_tag_string(mkvtag_context_t *ctx, const char *name,
                          const char *value) {
    if (!ctx || !name) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (!ctx->is_open) {
        return MKVTAG_ERR_NOT_OPEN;
    }

    if (!ctx->is_writable) {
        return MKVTAG_ERR_READ_ONLY;
    }

    /* Read existing tags or create new collection */
    mkvtag_collection_t *existing = NULL;
    int err = mkvtag_read_tags(ctx, &existing);

    mkvtag_collection_t *working;

    if (err == MKVTAG_OK && existing) {
        /* Clone existing tags - we need a mutable copy */
        /* For simplicity, serialize and re-parse, or just build new */
        working = mkvtag_collection_create(ctx);
        if (!working) return MKVTAG_ERR_NO_MEMORY;

        /* Copy all existing tags */
        const mkvtag_tag_t *src_tag = existing->tags;
        while (src_tag) {
            mkvtag_tag_t *new_tag = mkvtag_collection_add_tag(ctx, working,
                                                               src_tag->target_type);
            if (!new_tag) {
                mkvtag_collection_free(ctx, working);
                return MKVTAG_ERR_NO_MEMORY;
            }

            /* Copy target type string */
            if (src_tag->target_type_str) {
                new_tag->target_type_str = str_dup(src_tag->target_type_str);
            }

            /* Copy UID arrays */
            if (src_tag->track_uid_count > 0) {
                new_tag->track_uids = malloc(src_tag->track_uid_count * sizeof(uint64_t));
                if (new_tag->track_uids) {
                    memcpy(new_tag->track_uids, src_tag->track_uids,
                           src_tag->track_uid_count * sizeof(uint64_t));
                    new_tag->track_uid_count = src_tag->track_uid_count;
                }
            }

            /* Copy simple tags, skipping the one we're updating */
            const mkvtag_simple_tag_t *src_stag = src_tag->simple_tags;
            int found_in_this_tag = 0;
            while (src_stag) {
                int skip = 0;
                /* If this is an album-level tag, check for name match */
                if (src_tag->target_type == MKVTAG_TARGET_ALBUM &&
                    src_stag->name && str_casecmp(src_stag->name, name) == 0) {
                    found_in_this_tag = 1;
                    if (value == NULL) {
                        /* Removing this tag */
                        skip = 1;
                    } else {
                        /* Replace with new value */
                        mkvtag_tag_add_simple(ctx, new_tag, name, value);
                        skip = 1;
                    }
                }

                if (!skip) {
                    mkvtag_simple_tag_t *new_stag =
                        mkvtag_tag_add_simple(ctx, new_tag,
                                              src_stag->name ? src_stag->name : "",
                                              src_stag->value);
                    if (new_stag && src_stag->language) {
                        mkvtag_simple_tag_set_language(ctx, new_stag, src_stag->language);
                    }
                }

                src_stag = src_stag->next;
            }

            /* If we didn't find the tag to update, and this is album-level, add it */
            if (!found_in_this_tag && value != NULL &&
                src_tag->target_type == MKVTAG_TARGET_ALBUM) {
                /* We'll add it to the first album-level tag we find */
                /* (handled below if no album-level tag exists at all) */
            }

            src_tag = src_tag->next;
        }

        /* Check if we need to add a new tag entry */
        if (value != NULL) {
            int found_anywhere = 0;
            const mkvtag_tag_t *check = existing->tags;
            while (check) {
                if (check->target_type == MKVTAG_TARGET_ALBUM) {
                    const mkvtag_simple_tag_t *s = check->simple_tags;
                    while (s) {
                        if (s->name && str_casecmp(s->name, name) == 0) {
                            found_anywhere = 1;
                            break;
                        }
                        s = s->next;
                    }
                }
                check = check->next;
            }

            if (!found_anywhere) {
                /* Find or create an album-level tag to add the new simple tag to */
                mkvtag_tag_t *album_tag = working->tags;
                while (album_tag) {
                    if (album_tag->target_type == MKVTAG_TARGET_ALBUM) break;
                    album_tag = album_tag->next;
                }
                if (!album_tag) {
                    album_tag = mkvtag_collection_add_tag(ctx, working, MKVTAG_TARGET_ALBUM);
                }
                if (album_tag) {
                    mkvtag_tag_add_simple(ctx, album_tag, name, value);
                }
            }
        }
    } else {
        /* No existing tags - create new */
        working = mkvtag_collection_create(ctx);
        if (!working) return MKVTAG_ERR_NO_MEMORY;

        if (value != NULL) {
            mkvtag_tag_t *tag = mkvtag_collection_add_tag(ctx, working, MKVTAG_TARGET_ALBUM);
            if (tag) {
                mkvtag_tag_add_simple(ctx, tag, name, value);
            }
        }
    }

    /* Write the working collection */
    err = mkvtag_write_tags(ctx, working);
    mkvtag_collection_free(ctx, working);

    return err;
}

int mkvtag_remove_tag(mkvtag_context_t *ctx, const char *name) {
    return mkvtag_set_tag_string(ctx, name, NULL);
}

/*
 * Tag Collection Building
 */

mkvtag_collection_t *mkvtag_collection_create(mkvtag_context_t *ctx) {
    (void)ctx; /* May be used with custom allocator in the future */
    mkvtag_collection_t *coll = calloc(1, sizeof(mkvtag_collection_t));
    return coll;
}

void mkvtag_collection_free(mkvtag_context_t *ctx, mkvtag_collection_t *coll) {
    (void)ctx;
    mkv_tags_free_collection(coll);
}

mkvtag_tag_t *mkvtag_collection_add_tag(mkvtag_context_t *ctx,
                                         mkvtag_collection_t *coll,
                                         mkvtag_target_type_t type) {
    (void)ctx;
    if (!coll) return NULL;

    mkvtag_tag_t *tag = calloc(1, sizeof(mkvtag_tag_t));
    if (!tag) return NULL;

    tag->target_type = type;

    /* Append to end of list */
    if (!coll->tags) {
        coll->tags = tag;
    } else {
        mkvtag_tag_t *tail = coll->tags;
        while (tail->next) tail = tail->next;
        tail->next = tag;
    }
    coll->count++;

    return tag;
}

mkvtag_simple_tag_t *mkvtag_tag_add_simple(mkvtag_context_t *ctx,
                                            mkvtag_tag_t *tag,
                                            const char *name,
                                            const char *value) {
    (void)ctx;
    if (!tag || !name) return NULL;

    mkvtag_simple_tag_t *stag = calloc(1, sizeof(mkvtag_simple_tag_t));
    if (!stag) return NULL;

    stag->name = str_dup(name);
    if (!stag->name) {
        free(stag);
        return NULL;
    }

    if (value) {
        stag->value = str_dup(value);
        if (!stag->value) {
            free(stag->name);
            free(stag);
            return NULL;
        }
    }

    stag->is_default = 1;

    /* Append to end of list */
    if (!tag->simple_tags) {
        tag->simple_tags = stag;
    } else {
        mkvtag_simple_tag_t *tail = tag->simple_tags;
        while (tail->next) tail = tail->next;
        tail->next = stag;
    }

    return stag;
}

mkvtag_simple_tag_t *mkvtag_simple_tag_add_nested(mkvtag_context_t *ctx,
                                                   mkvtag_simple_tag_t *parent,
                                                   const char *name,
                                                   const char *value) {
    (void)ctx;
    if (!parent || !name) return NULL;

    mkvtag_simple_tag_t *stag = calloc(1, sizeof(mkvtag_simple_tag_t));
    if (!stag) return NULL;

    stag->name = str_dup(name);
    if (!stag->name) {
        free(stag);
        return NULL;
    }

    if (value) {
        stag->value = str_dup(value);
        if (!stag->value) {
            free(stag->name);
            free(stag);
            return NULL;
        }
    }

    stag->is_default = 1;

    /* Append to nested list */
    if (!parent->nested) {
        parent->nested = stag;
    } else {
        mkvtag_simple_tag_t *tail = parent->nested;
        while (tail->next) tail = tail->next;
        tail->next = stag;
    }

    return stag;
}

int mkvtag_simple_tag_set_language(mkvtag_context_t *ctx,
                                   mkvtag_simple_tag_t *simple_tag,
                                   const char *language) {
    (void)ctx;
    if (!simple_tag || !language) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    free(simple_tag->language);
    simple_tag->language = str_dup(language);
    if (!simple_tag->language) {
        return MKVTAG_ERR_NO_MEMORY;
    }

    return MKVTAG_OK;
}

int mkvtag_tag_add_track_uid(mkvtag_context_t *ctx, mkvtag_tag_t *tag,
                             uint64_t uid) {
    (void)ctx;
    if (!tag) return MKVTAG_ERR_INVALID_ARG;

    uint64_t *new_uids = realloc(tag->track_uids,
                                 (tag->track_uid_count + 1) * sizeof(uint64_t));
    if (!new_uids) return MKVTAG_ERR_NO_MEMORY;

    tag->track_uids = new_uids;
    tag->track_uids[tag->track_uid_count++] = uid;

    return MKVTAG_OK;
}
