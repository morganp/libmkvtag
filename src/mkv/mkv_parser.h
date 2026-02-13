/*
 * mkv_parser.h - MKV structure navigation and parsing
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef MKV_PARSER_H
#define MKV_PARSER_H

#include <stddef.h>
#include <stdint.h>
#include <tag_common/file_io.h>
#include "../ebml/ebml_reader.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum number of cached element positions */
#define MKV_MAX_CACHED_ELEMENTS 32

/**
 * Cached element position entry.
 */
typedef struct mkv_element_pos {
    uint32_t id;            /* Element ID */
    int64_t offset;         /* Absolute file offset of element header */
    uint64_t size;          /* Element content size */
} mkv_element_pos_t;

/**
 * Parsed MKV file structure.
 */
typedef struct mkv_file {
    file_handle_t *handle;

    /* EBML header info */
    int ebml_version;
    int ebml_read_version;
    char doctype[32];
    int doctype_version;
    int doctype_read_version;

    /* Segment info */
    int64_t segment_offset;         /* Start of Segment element header */
    int64_t segment_data_offset;    /* Start of Segment content */
    uint64_t segment_size;          /* Segment content size */
    int segment_unknown_size;       /* Whether Segment has unknown size */

    /* Element position cache */
    mkv_element_pos_t elements[MKV_MAX_CACHED_ELEMENTS];
    int element_count;

    /* Specific element positions (populated from SeekHead or scanning) */
    int64_t seekhead_offset;
    int64_t info_offset;
    int64_t tracks_offset;
    int64_t cues_offset;
    int64_t tags_offset;
    int64_t chapters_offset;
    int64_t attachments_offset;
    int64_t clusters_offset;        /* First cluster offset */

    /* Void elements (for potential tag writing space) */
    int64_t void_offset;            /* Largest Void element offset */
    uint64_t void_size;             /* Largest Void element total size (header+data) */
} mkv_file_t;

/**
 * Initialize an mkv_file structure.
 *
 * @param mkv The structure to initialize
 */
void mkv_file_init(mkv_file_t *mkv);

/**
 * Parse EBML header and validate it's a Matroska file.
 *
 * @param mkv The MKV file structure
 * @param handle The open file handle
 * @return 0 on success, negative error code on failure
 */
int mkv_parse_header(mkv_file_t *mkv, file_handle_t *handle);

/**
 * Locate the Segment element and parse its structure.
 * Uses SeekHead when available, falls back to scanning.
 *
 * @param mkv The MKV file structure (must have been parsed with mkv_parse_header)
 * @return 0 on success, negative error code on failure
 */
int mkv_parse_structure(mkv_file_t *mkv);

/**
 * Find an element by ID within a parent element.
 * Scans from the current file position.
 *
 * @param mkv The MKV file structure
 * @param parent The parent element to search within
 * @param target_id The element ID to find
 * @param found Output: the found element
 * @return 0 on success, MKVTAG_ERR_TAG_NOT_FOUND if not found, or another error
 */
int mkv_find_element(mkv_file_t *mkv, const ebml_element_t *parent,
                     uint32_t target_id, ebml_element_t *found);

/**
 * Get the absolute offset of an element from its position relative to Segment data.
 *
 * @param mkv The MKV file structure
 * @param relative_position Position relative to Segment data start
 * @return Absolute file offset
 */
int64_t mkv_segment_to_absolute(const mkv_file_t *mkv, uint64_t relative_position);

/**
 * Get the position of an element relative to Segment data start.
 *
 * @param mkv The MKV file structure
 * @param absolute_offset Absolute file offset
 * @return Position relative to Segment data start
 */
uint64_t mkv_absolute_to_segment(const mkv_file_t *mkv, int64_t absolute_offset);

/**
 * Cache an element position.
 *
 * @param mkv The MKV file structure
 * @param id Element ID
 * @param offset Absolute file offset
 * @param size Element content size
 */
void mkv_cache_element(mkv_file_t *mkv, uint32_t id, int64_t offset, uint64_t size);

/**
 * Look up a cached element position.
 *
 * @param mkv The MKV file structure
 * @param id Element ID
 * @return Pointer to cached position, or NULL if not found
 */
const mkv_element_pos_t *mkv_lookup_element(const mkv_file_t *mkv, uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* MKV_PARSER_H */
