/*
 * mkv_tags.c - MKV tag element handling (parsing and serialization)
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "mkv_tags.h"
#include "../ebml/ebml_ids.h"
#include "../ebml/ebml_reader.h"
#include "../ebml/ebml_writer.h"
#include "../ebml/ebml_vint.h"
#include <tag_common/string_util.h>
#include "../include/mkvtag/mkvtag_error.h"

#include <stdlib.h>
#include <string.h>

/*
 * Parsing
 */

static mkvtag_simple_tag_t *parse_simple_tag(mkv_file_t *mkv,
                                              const ebml_element_t *simple_elem);

static int parse_targets(mkv_file_t *mkv, const ebml_element_t *targets_elem,
                         mkvtag_tag_t *tag) {
    file_handle_t *handle = mkv->handle;

    int err = file_seek(handle, targets_elem->data_offset);
    if (err != MKVTAG_OK) return err;

    /* Set default target type */
    tag->target_type = MKVTAG_TARGET_ALBUM;

    while (!ebml_at_element_end(handle, targets_elem)) {
        ebml_element_t child;
        err = ebml_read_element_header(handle, &child);
        if (err != MKVTAG_OK) break;

        switch (child.id) {
            case MKV_ID_TARGET_TYPE_VALUE: {
                uint64_t val;
                err = ebml_read_uint(handle, &child, &val);
                if (err == MKVTAG_OK) {
                    tag->target_type = (mkvtag_target_type_t)val;
                }
                break;
            }
            case MKV_ID_TARGET_TYPE: {
                ebml_read_string_alloc(handle, &child, &tag->target_type_str);
                break;
            }
            case MKV_ID_TAG_TRACK_UID: {
                uint64_t uid;
                err = ebml_read_uint(handle, &child, &uid);
                if (err == MKVTAG_OK) {
                    uint64_t *new_uids = realloc(tag->track_uids,
                        (tag->track_uid_count + 1) * sizeof(uint64_t));
                    if (new_uids) {
                        tag->track_uids = new_uids;
                        tag->track_uids[tag->track_uid_count++] = uid;
                    }
                }
                break;
            }
            case MKV_ID_TAG_EDITION_UID: {
                uint64_t uid;
                err = ebml_read_uint(handle, &child, &uid);
                if (err == MKVTAG_OK) {
                    uint64_t *new_uids = realloc(tag->edition_uids,
                        (tag->edition_uid_count + 1) * sizeof(uint64_t));
                    if (new_uids) {
                        tag->edition_uids = new_uids;
                        tag->edition_uids[tag->edition_uid_count++] = uid;
                    }
                }
                break;
            }
            case MKV_ID_TAG_CHAPTER_UID: {
                uint64_t uid;
                err = ebml_read_uint(handle, &child, &uid);
                if (err == MKVTAG_OK) {
                    uint64_t *new_uids = realloc(tag->chapter_uids,
                        (tag->chapter_uid_count + 1) * sizeof(uint64_t));
                    if (new_uids) {
                        tag->chapter_uids = new_uids;
                        tag->chapter_uids[tag->chapter_uid_count++] = uid;
                    }
                }
                break;
            }
            case MKV_ID_TAG_ATTACHMENT_UID: {
                uint64_t uid;
                err = ebml_read_uint(handle, &child, &uid);
                if (err == MKVTAG_OK) {
                    uint64_t *new_uids = realloc(tag->attachment_uids,
                        (tag->attachment_uid_count + 1) * sizeof(uint64_t));
                    if (new_uids) {
                        tag->attachment_uids = new_uids;
                        tag->attachment_uids[tag->attachment_uid_count++] = uid;
                    }
                }
                break;
            }
            default:
                break;
        }

        ebml_skip_element(handle, &child);
    }

    return MKVTAG_OK;
}

static mkvtag_simple_tag_t *parse_simple_tag(mkv_file_t *mkv,
                                              const ebml_element_t *simple_elem) {
    file_handle_t *handle = mkv->handle;

    mkvtag_simple_tag_t *stag = calloc(1, sizeof(mkvtag_simple_tag_t));
    if (!stag) return NULL;

    stag->is_default = 1; /* Default per spec */

    int err = file_seek(handle, simple_elem->data_offset);
    if (err != MKVTAG_OK) {
        free(stag);
        return NULL;
    }

    while (!ebml_at_element_end(handle, simple_elem)) {
        ebml_element_t child;
        err = ebml_read_element_header(handle, &child);
        if (err != MKVTAG_OK) break;

        switch (child.id) {
            case MKV_ID_TAG_NAME:
                ebml_read_string_alloc(handle, &child, &stag->name);
                break;
            case MKV_ID_TAG_STRING:
                ebml_read_string_alloc(handle, &child, &stag->value);
                break;
            case MKV_ID_TAG_BINARY:
                ebml_read_binary_alloc(handle, &child, &stag->binary, &stag->binary_size);
                break;
            case MKV_ID_TAG_LANGUAGE:
            case MKV_ID_TAG_LANGUAGE_BCP47:
                ebml_read_string_alloc(handle, &child, &stag->language);
                break;
            case MKV_ID_TAG_DEFAULT: {
                uint64_t val;
                err = ebml_read_uint(handle, &child, &val);
                if (err == MKVTAG_OK) {
                    stag->is_default = (int)val;
                }
                break;
            }
            case MKV_ID_SIMPLE_TAG: {
                /* Nested SimpleTag */
                mkvtag_simple_tag_t *nested = parse_simple_tag(mkv, &child);
                if (nested) {
                    nested->next = stag->nested;
                    stag->nested = nested;
                }
                break;
            }
            default:
                break;
        }

        ebml_skip_element(handle, &child);
    }

    return stag;
}

static mkvtag_tag_t *parse_tag(mkv_file_t *mkv, const ebml_element_t *tag_elem) {
    file_handle_t *handle = mkv->handle;

    mkvtag_tag_t *tag = calloc(1, sizeof(mkvtag_tag_t));
    if (!tag) return NULL;

    tag->target_type = MKVTAG_TARGET_ALBUM; /* Default */

    int err = file_seek(handle, tag_elem->data_offset);
    if (err != MKVTAG_OK) {
        free(tag);
        return NULL;
    }

    while (!ebml_at_element_end(handle, tag_elem)) {
        ebml_element_t child;
        err = ebml_read_element_header(handle, &child);
        if (err != MKVTAG_OK) break;

        switch (child.id) {
            case MKV_ID_TARGETS:
                parse_targets(mkv, &child, tag);
                break;
            case MKV_ID_SIMPLE_TAG: {
                mkvtag_simple_tag_t *stag = parse_simple_tag(mkv, &child);
                if (stag) {
                    /* Prepend to list */
                    stag->next = tag->simple_tags;
                    tag->simple_tags = stag;
                }
                break;
            }
            default:
                break;
        }

        ebml_skip_element(handle, &child);
    }

    return tag;
}

int mkv_tags_parse(mkv_file_t *mkv, const ebml_element_t *tags_element,
                   mkvtag_collection_t **collection) {
    if (!mkv || !tags_element || !collection) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    mkvtag_collection_t *coll = calloc(1, sizeof(mkvtag_collection_t));
    if (!coll) return MKVTAG_ERR_NO_MEMORY;

    file_handle_t *handle = mkv->handle;

    int err = file_seek(handle, tags_element->data_offset);
    if (err != MKVTAG_OK) {
        free(coll);
        return err;
    }

    mkvtag_tag_t *tail = NULL;

    while (!ebml_at_element_end(handle, tags_element)) {
        ebml_element_t child;
        err = ebml_read_element_header(handle, &child);
        if (err != MKVTAG_OK) break;

        if (child.id == MKV_ID_TAG) {
            mkvtag_tag_t *tag = parse_tag(mkv, &child);
            if (tag) {
                /* Append to list (maintain order) */
                if (tail) {
                    tail->next = tag;
                } else {
                    coll->tags = tag;
                }
                tail = tag;
                coll->count++;
            }
        }

        ebml_skip_element(handle, &child);
    }

    *collection = coll;
    return MKVTAG_OK;
}

/*
 * Serialization
 */

static int serialize_simple_tag(const mkvtag_simple_tag_t *stag, dyn_buffer_t *buf) {
    if (!stag || !buf) return MKVTAG_ERR_INVALID_ARG;

    /* Build SimpleTag content */
    dyn_buffer_t content;
    buffer_init(&content);

    /* TagName (required) */
    int err;
    if (stag->name) {
        err = ebml_write_string_element(&content, MKV_ID_TAG_NAME, stag->name);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* TagLanguage */
    if (stag->language) {
        err = ebml_write_string_element(&content, MKV_ID_TAG_LANGUAGE, stag->language);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* TagDefault */
    if (!stag->is_default) {
        err = ebml_write_uint_element(&content, MKV_ID_TAG_DEFAULT, 0);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* TagString */
    if (stag->value) {
        err = ebml_write_string_element(&content, MKV_ID_TAG_STRING, stag->value);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* TagBinary */
    if (stag->binary && stag->binary_size > 0) {
        err = ebml_write_binary_element(&content, MKV_ID_TAG_BINARY,
                                        stag->binary, stag->binary_size);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* Nested SimpleTags */
    const mkvtag_simple_tag_t *nested = stag->nested;
    while (nested) {
        err = serialize_simple_tag(nested, &content);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
        nested = nested->next;
    }

    /* Write SimpleTag master element */
    err = ebml_write_master_element_header(buf, MKV_ID_SIMPLE_TAG, content.size);
    if (err != MKVTAG_OK) { buffer_free(&content); return err; }

    err = buffer_append(buf, content.data, content.size);
    buffer_free(&content);
    return err;
}

static int serialize_targets(const mkvtag_tag_t *tag, dyn_buffer_t *buf) {
    if (!tag || !buf) return MKVTAG_ERR_INVALID_ARG;

    dyn_buffer_t content;
    buffer_init(&content);

    /* TargetTypeValue */
    int err = ebml_write_uint_element(&content, MKV_ID_TARGET_TYPE_VALUE,
                                  (uint64_t)tag->target_type);
    if (err != MKVTAG_OK) { buffer_free(&content); return err; }

    /* TargetType string */
    if (tag->target_type_str) {
        err = ebml_write_string_element(&content, MKV_ID_TARGET_TYPE, tag->target_type_str);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* Track UIDs */
    for (size_t i = 0; i < tag->track_uid_count; i++) {
        err = ebml_write_uint_element(&content, MKV_ID_TAG_TRACK_UID, tag->track_uids[i]);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* Edition UIDs */
    for (size_t i = 0; i < tag->edition_uid_count; i++) {
        err = ebml_write_uint_element(&content, MKV_ID_TAG_EDITION_UID, tag->edition_uids[i]);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* Chapter UIDs */
    for (size_t i = 0; i < tag->chapter_uid_count; i++) {
        err = ebml_write_uint_element(&content, MKV_ID_TAG_CHAPTER_UID, tag->chapter_uids[i]);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* Attachment UIDs */
    for (size_t i = 0; i < tag->attachment_uid_count; i++) {
        err = ebml_write_uint_element(&content, MKV_ID_TAG_ATTACHMENT_UID, tag->attachment_uids[i]);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
    }

    /* Write Targets master element */
    err = ebml_write_master_element_header(buf, MKV_ID_TARGETS, content.size);
    if (err != MKVTAG_OK) { buffer_free(&content); return err; }

    err = buffer_append(buf, content.data, content.size);
    buffer_free(&content);
    return err;
}

static int serialize_tag(const mkvtag_tag_t *tag, dyn_buffer_t *buf) {
    if (!tag || !buf) return MKVTAG_ERR_INVALID_ARG;

    dyn_buffer_t content;
    buffer_init(&content);

    /* Targets */
    int err = serialize_targets(tag, &content);
    if (err != MKVTAG_OK) { buffer_free(&content); return err; }

    /* SimpleTags */
    const mkvtag_simple_tag_t *stag = tag->simple_tags;
    while (stag) {
        err = serialize_simple_tag(stag, &content);
        if (err != MKVTAG_OK) { buffer_free(&content); return err; }
        stag = stag->next;
    }

    /* Write Tag master element */
    err = ebml_write_master_element_header(buf, MKV_ID_TAG, content.size);
    if (err != MKVTAG_OK) { buffer_free(&content); return err; }

    err = buffer_append(buf, content.data, content.size);
    buffer_free(&content);
    return err;
}

int mkv_tags_serialize(const mkvtag_collection_t *collection, dyn_buffer_t *buf) {
    if (!collection || !buf) return MKVTAG_ERR_INVALID_ARG;

    const mkvtag_tag_t *tag = collection->tags;
    while (tag) {
        int err = serialize_tag(tag, buf);
        if (err != MKVTAG_OK) return err;
        tag = tag->next;
    }

    return MKVTAG_OK;
}

size_t mkv_tags_total_size(const mkvtag_collection_t *collection) {
    if (!collection) return 0;

    /* Serialize to get exact size */
    dyn_buffer_t content;
    buffer_init(&content);

    if (mkv_tags_serialize(collection, &content) != MKVTAG_OK) {
        buffer_free(&content);
        return 0;
    }

    size_t content_size = content.size;
    buffer_free(&content);

    /* Total = Tags header + content */
    return ebml_master_header_size(MKV_ID_TAGS, content_size) + content_size;
}

/*
 * Memory management
 */

void mkv_tags_free_simple_tag(mkvtag_simple_tag_t *tag) {
    while (tag) {
        mkvtag_simple_tag_t *next = tag->next;
        free(tag->name);
        free(tag->value);
        free(tag->binary);
        free(tag->language);
        mkv_tags_free_simple_tag(tag->nested);
        free(tag);
        tag = next;
    }
}

static void free_tag(mkvtag_tag_t *tag) {
    while (tag) {
        mkvtag_tag_t *next = tag->next;
        free(tag->target_type_str);
        free(tag->track_uids);
        free(tag->edition_uids);
        free(tag->chapter_uids);
        free(tag->attachment_uids);
        mkv_tags_free_simple_tag(tag->simple_tags);
        free(tag);
        tag = next;
    }
}

void mkv_tags_free_collection(mkvtag_collection_t *collection) {
    if (!collection) return;
    free_tag(collection->tags);
    free(collection);
}
