/*
 * mkvtag_types.h - Public type definitions for libmkvtag
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef MKVTAG_TYPES_H
#define MKVTAG_TYPES_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Target type values for tag scoping.
 * These correspond to Matroska TargetTypeValue.
 */
typedef enum mkvtag_target_type {
    MKVTAG_TARGET_COLLECTION = 70,  /* Album, concert, movie series */
    MKVTAG_TARGET_EDITION    = 60,  /* Issue, volume, opus */
    MKVTAG_TARGET_ALBUM      = 50,  /* Album, opera, movie, episode */
    MKVTAG_TARGET_PART       = 40,  /* Part, session */
    MKVTAG_TARGET_TRACK      = 30,  /* Track, song, chapter */
    MKVTAG_TARGET_SUBTRACK   = 20,  /* Subtrack, movement, scene */
    MKVTAG_TARGET_SHOT       = 10,  /* Shot */
} mkvtag_target_type_t;

/**
 * A simple tag (name/value pair with optional language).
 */
typedef struct mkvtag_simple_tag {
    char *name;                         /* Tag name (UTF-8) */
    char *value;                        /* Tag string value (UTF-8, may be NULL) */
    uint8_t *binary;                    /* Binary value (may be NULL) */
    size_t binary_size;                 /* Size of binary data */
    char *language;                     /* Language code (may be NULL, defaults to "und") */
    int is_default;                     /* Whether this is the default for the language */
    struct mkvtag_simple_tag *nested;   /* First nested SimpleTag (may be NULL) */
    struct mkvtag_simple_tag *next;     /* Next sibling SimpleTag */
} mkvtag_simple_tag_t;

/**
 * A tag with targets and simple tags.
 */
typedef struct mkvtag_tag {
    /* Target specification */
    mkvtag_target_type_t target_type;   /* Target type value */
    char *target_type_str;              /* Target type string (may be NULL) */
    uint64_t *track_uids;               /* Array of target track UIDs */
    size_t track_uid_count;             /* Number of track UIDs */
    uint64_t *edition_uids;             /* Array of target edition UIDs */
    size_t edition_uid_count;           /* Number of edition UIDs */
    uint64_t *chapter_uids;             /* Array of target chapter UIDs */
    size_t chapter_uid_count;           /* Number of chapter UIDs */
    uint64_t *attachment_uids;          /* Array of target attachment UIDs */
    size_t attachment_uid_count;        /* Number of attachment UIDs */

    /* Simple tags */
    mkvtag_simple_tag_t *simple_tags;   /* Linked list of simple tags */

    /* Linked list */
    struct mkvtag_tag *next;            /* Next tag in collection */
} mkvtag_tag_t;

/**
 * A collection of tags (represents a Tags element).
 */
typedef struct mkvtag_collection {
    mkvtag_tag_t *tags;                 /* Linked list of tags */
    size_t count;                       /* Number of tags */
} mkvtag_collection_t;

/**
 * Custom allocator interface.
 * Set any function to NULL to use the default (malloc/realloc/free).
 */
typedef struct mkvtag_allocator {
    void *(*alloc)(size_t size, void *user_data);
    void *(*realloc)(void *ptr, size_t size, void *user_data);
    void (*free)(void *ptr, void *user_data);
    void *user_data;
} mkvtag_allocator_t;

/**
 * Opaque context structure.
 */
typedef struct mkvtag_context mkvtag_context_t;

#ifdef __cplusplus
}
#endif

#endif /* MKVTAG_TYPES_H */
