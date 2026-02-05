/*
 * mkvtag.h - Main public API for libmkvtag
 *
 * A pure C library for reading and writing MKV file metadata tags
 * without loading entire files into memory.
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef MKVTAG_H
#define MKVTAG_H

#include "mkvtag_types.h"
#include "mkvtag_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Library version */
#define MKVTAG_VERSION_MAJOR 1
#define MKVTAG_VERSION_MINOR 0
#define MKVTAG_VERSION_PATCH 0

/**
 * Get the library version as a string.
 *
 * @return Version string in the format "major.minor.patch"
 */
const char *mkvtag_version(void);

/*
 * Context Management
 */

/**
 * Create a new mkvtag context.
 *
 * @param allocator Custom allocator, or NULL for default (malloc/free)
 * @return New context, or NULL on allocation failure
 */
mkvtag_context_t *mkvtag_create(const mkvtag_allocator_t *allocator);

/**
 * Destroy a context and free all associated resources.
 * This will also close any open file.
 *
 * @param ctx The context to destroy (may be NULL)
 */
void mkvtag_destroy(mkvtag_context_t *ctx);

/*
 * File Operations
 */

/**
 * Open an MKV file for reading.
 *
 * @param ctx The context
 * @param path Path to the MKV file (UTF-8)
 * @return MKVTAG_OK on success, or an error code
 */
int mkvtag_open(mkvtag_context_t *ctx, const char *path);

/**
 * Open an MKV file for reading and writing.
 *
 * @param ctx The context
 * @param path Path to the MKV file (UTF-8)
 * @return MKVTAG_OK on success, or an error code
 */
int mkvtag_open_rw(mkvtag_context_t *ctx, const char *path);

/**
 * Close the currently open file.
 *
 * @param ctx The context
 */
void mkvtag_close(mkvtag_context_t *ctx);

/**
 * Check if a file is currently open.
 *
 * @param ctx The context
 * @return Non-zero if a file is open, 0 otherwise
 */
int mkvtag_is_open(const mkvtag_context_t *ctx);

/*
 * Tag Reading
 */

/**
 * Read all tags from the file.
 *
 * @param ctx The context
 * @param tags Output: pointer to receive the tag collection
 * @return MKVTAG_OK on success, or an error code
 *
 * The returned collection is owned by the context and will be freed
 * when the context is destroyed or when mkvtag_close() is called.
 * Do not free it manually.
 */
int mkvtag_read_tags(mkvtag_context_t *ctx, mkvtag_collection_t **tags);

/**
 * Read a single tag value by name.
 *
 * This is a convenience function that searches for a SimpleTag with the
 * given name at the album/movie level (target type 50) and returns its
 * string value.
 *
 * @param ctx The context
 * @param name The tag name to search for (case-insensitive)
 * @param value Buffer to receive the value (UTF-8)
 * @param size Size of the value buffer
 * @return MKVTAG_OK on success, MKVTAG_ERR_TAG_NOT_FOUND if not found,
 *         MKVTAG_ERR_TAG_TOO_LARGE if buffer is too small, or another error
 */
int mkvtag_read_tag_string(mkvtag_context_t *ctx, const char *name,
                           char *value, size_t size);

/*
 * Tag Writing
 */

/**
 * Write a complete tag collection to the file.
 *
 * This replaces all existing tags in the file with the provided collection.
 *
 * @param ctx The context (must be opened with mkvtag_open_rw)
 * @param tags The tag collection to write
 * @return MKVTAG_OK on success, or an error code
 */
int mkvtag_write_tags(mkvtag_context_t *ctx, const mkvtag_collection_t *tags);

/**
 * Set a single tag value by name.
 *
 * This is a convenience function that sets a SimpleTag with the given
 * name at the album/movie level (target type 50). If a tag with this
 * name already exists, it will be updated; otherwise, a new tag will
 * be created.
 *
 * @param ctx The context (must be opened with mkvtag_open_rw)
 * @param name The tag name (UTF-8)
 * @param value The tag value (UTF-8), or NULL to remove the tag
 * @return MKVTAG_OK on success, or an error code
 */
int mkvtag_set_tag_string(mkvtag_context_t *ctx, const char *name,
                          const char *value);

/**
 * Remove a tag by name.
 *
 * @param ctx The context (must be opened with mkvtag_open_rw)
 * @param name The tag name to remove (case-insensitive)
 * @return MKVTAG_OK on success, MKVTAG_ERR_TAG_NOT_FOUND if not found,
 *         or another error
 */
int mkvtag_remove_tag(mkvtag_context_t *ctx, const char *name);

/*
 * Tag Collection Building
 */

/**
 * Create a new empty tag collection.
 *
 * @param ctx The context
 * @return New collection, or NULL on allocation failure
 *
 * The caller is responsible for freeing the collection with
 * mkvtag_collection_free().
 */
mkvtag_collection_t *mkvtag_collection_create(mkvtag_context_t *ctx);

/**
 * Free a tag collection.
 *
 * @param ctx The context
 * @param coll The collection to free (may be NULL)
 */
void mkvtag_collection_free(mkvtag_context_t *ctx, mkvtag_collection_t *coll);

/**
 * Add a new tag to a collection.
 *
 * @param ctx The context
 * @param coll The collection
 * @param type The target type for the tag
 * @return New tag, or NULL on allocation failure
 */
mkvtag_tag_t *mkvtag_collection_add_tag(mkvtag_context_t *ctx,
                                         mkvtag_collection_t *coll,
                                         mkvtag_target_type_t type);

/**
 * Add a simple tag (name/value pair) to a tag.
 *
 * @param ctx The context
 * @param tag The parent tag
 * @param name The tag name (UTF-8)
 * @param value The tag value (UTF-8, may be NULL)
 * @return New simple tag, or NULL on allocation failure
 */
mkvtag_simple_tag_t *mkvtag_tag_add_simple(mkvtag_context_t *ctx,
                                            mkvtag_tag_t *tag,
                                            const char *name,
                                            const char *value);

/**
 * Add a nested simple tag to an existing simple tag.
 *
 * @param ctx The context
 * @param parent The parent simple tag
 * @param name The tag name (UTF-8)
 * @param value The tag value (UTF-8, may be NULL)
 * @return New simple tag, or NULL on allocation failure
 */
mkvtag_simple_tag_t *mkvtag_simple_tag_add_nested(mkvtag_context_t *ctx,
                                                   mkvtag_simple_tag_t *parent,
                                                   const char *name,
                                                   const char *value);

/**
 * Set the language for a simple tag.
 *
 * @param ctx The context
 * @param simple_tag The simple tag
 * @param language The language code (e.g., "eng", "und")
 * @return MKVTAG_OK on success, or an error code
 */
int mkvtag_simple_tag_set_language(mkvtag_context_t *ctx,
                                   mkvtag_simple_tag_t *simple_tag,
                                   const char *language);

/**
 * Add a track UID target to a tag.
 *
 * @param ctx The context
 * @param tag The tag
 * @param uid The track UID
 * @return MKVTAG_OK on success, or an error code
 */
int mkvtag_tag_add_track_uid(mkvtag_context_t *ctx, mkvtag_tag_t *tag,
                             uint64_t uid);

#ifdef __cplusplus
}
#endif

#endif /* MKVTAG_H */
