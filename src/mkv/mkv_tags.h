/*
 * mkv_tags.h - MKV tag element handling
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef MKV_TAGS_H
#define MKV_TAGS_H

#include "../include/mkvtag/mkvtag_types.h"
#include "mkv_parser.h"
#include "../util/buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse Tags element into a mkvtag_collection_t structure.
 *
 * @param mkv The MKV file structure
 * @param tags_element The Tags element to parse
 * @param collection Output: the parsed collection (caller must free with mkv_tags_free_collection)
 * @return 0 on success, negative error code on failure
 */
int mkv_tags_parse(mkv_file_t *mkv, const ebml_element_t *tags_element,
                   mkvtag_collection_t **collection);

/**
 * Serialize a tag collection to EBML binary format.
 * Produces the content of a Tags element (without the Tags header itself).
 *
 * @param collection The tag collection to serialize
 * @param buf Output buffer
 * @return 0 on success, negative error code on failure
 */
int mkv_tags_serialize(const mkvtag_collection_t *collection, dyn_buffer_t *buf);

/**
 * Calculate the serialized size of a Tags element (including header).
 *
 * @param collection The tag collection
 * @return Total size in bytes, or 0 on error
 */
size_t mkv_tags_total_size(const mkvtag_collection_t *collection);

/**
 * Free a tag collection and all associated memory.
 *
 * @param collection The collection to free (may be NULL)
 */
void mkv_tags_free_collection(mkvtag_collection_t *collection);

/**
 * Free a simple tag and all its siblings/children.
 *
 * @param tag The simple tag to free (may be NULL)
 */
void mkv_tags_free_simple_tag(mkvtag_simple_tag_t *tag);

#ifdef __cplusplus
}
#endif

#endif /* MKV_TAGS_H */
