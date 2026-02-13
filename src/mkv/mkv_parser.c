/*
 * mkv_parser.c - MKV structure navigation and parsing
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "mkv_parser.h"
#include "mkv_seekhead.h"
#include "../ebml/ebml_ids.h"
#include "../ebml/ebml_reader.h"
#include "../ebml/ebml_vint.h"
#include "../include/mkvtag/mkvtag_error.h"

#include <string.h>

void mkv_file_init(mkv_file_t *mkv) {
    if (!mkv) return;
    memset(mkv, 0, sizeof(*mkv));
    mkv->seekhead_offset = -1;
    mkv->info_offset = -1;
    mkv->tracks_offset = -1;
    mkv->cues_offset = -1;
    mkv->tags_offset = -1;
    mkv->chapters_offset = -1;
    mkv->attachments_offset = -1;
    mkv->clusters_offset = -1;
    mkv->void_offset = -1;
}

int mkv_parse_header(mkv_file_t *mkv, file_handle_t *handle) {
    if (!mkv || !handle) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    mkv->handle = handle;

    /* Seek to beginning */
    int err = file_seek(handle, 0);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* Read EBML header element */
    ebml_element_t header;
    err = ebml_read_element_header(handle, &header);
    if (err != MKVTAG_OK) {
        return MKVTAG_ERR_NOT_EBML;
    }

    if (header.id != EBML_ID_EBML) {
        return MKVTAG_ERR_NOT_EBML;
    }

    /* Set defaults */
    mkv->ebml_version = 1;
    mkv->ebml_read_version = 1;
    mkv->doctype_version = 1;
    mkv->doctype_read_version = 1;
    mkv->doctype[0] = '\0';

    /* Parse EBML header children */
    while (!ebml_at_element_end(handle, &header)) {
        ebml_element_t child;
        err = ebml_read_element_header(handle, &child);
        if (err != MKVTAG_OK) {
            break;
        }

        switch (child.id) {
            case EBML_ID_VERSION: {
                uint64_t val;
                err = ebml_read_uint(handle, &child, &val);
                if (err == MKVTAG_OK) {
                    mkv->ebml_version = (int)val;
                }
                break;
            }
            case EBML_ID_READ_VERSION: {
                uint64_t val;
                err = ebml_read_uint(handle, &child, &val);
                if (err == MKVTAG_OK) {
                    mkv->ebml_read_version = (int)val;
                }
                break;
            }
            case EBML_ID_DOCTYPE: {
                ebml_read_string(handle, &child, mkv->doctype, sizeof(mkv->doctype));
                break;
            }
            case EBML_ID_DOCTYPE_VERSION: {
                uint64_t val;
                err = ebml_read_uint(handle, &child, &val);
                if (err == MKVTAG_OK) {
                    mkv->doctype_version = (int)val;
                }
                break;
            }
            case EBML_ID_DOCTYPE_READ_VER: {
                uint64_t val;
                err = ebml_read_uint(handle, &child, &val);
                if (err == MKVTAG_OK) {
                    mkv->doctype_read_version = (int)val;
                }
                break;
            }
            default:
                break;
        }

        /* Skip to end of child element */
        err = ebml_skip_element(handle, &child);
        if (err != MKVTAG_OK) {
            return err;
        }
    }

    /* Validate DocType */
    if (strcmp(mkv->doctype, "matroska") != 0 &&
        strcmp(mkv->doctype, "webm") != 0) {
        return MKVTAG_ERR_NOT_MKV;
    }

    return MKVTAG_OK;
}

/**
 * Store the offset for a known top-level element ID.
 */
static void mkv_store_element_offset(mkv_file_t *mkv, uint32_t id, int64_t offset) {
    switch (id) {
        case MKV_ID_SEEKHEAD:    mkv->seekhead_offset = offset; break;
        case MKV_ID_INFO:        mkv->info_offset = offset; break;
        case MKV_ID_TRACKS:      mkv->tracks_offset = offset; break;
        case MKV_ID_CUES:        mkv->cues_offset = offset; break;
        case MKV_ID_TAGS:        mkv->tags_offset = offset; break;
        case MKV_ID_CHAPTERS:    mkv->chapters_offset = offset; break;
        case MKV_ID_ATTACHMENTS: mkv->attachments_offset = offset; break;
        case MKV_ID_CLUSTER:
            if (mkv->clusters_offset < 0) {
                mkv->clusters_offset = offset;
            }
            break;
        default:
            break;
    }
}

/**
 * Parse the SeekHead to populate element positions.
 */
static int mkv_parse_seekhead(mkv_file_t *mkv, const ebml_element_t *seekhead) {
    file_handle_t *handle = mkv->handle;

    int err = file_seek(handle, seekhead->data_offset);
    if (err != MKVTAG_OK) return err;

    while (!ebml_at_element_end(handle, seekhead)) {
        ebml_element_t seek_elem;
        err = ebml_read_element_header(handle, &seek_elem);
        if (err != MKVTAG_OK) break;

        if (seek_elem.id != MKV_ID_SEEK) {
            ebml_skip_element(handle, &seek_elem);
            continue;
        }

        /* Parse Seek entry */
        uint32_t seek_id = 0;
        uint64_t seek_position = 0;

        while (!ebml_at_element_end(handle, &seek_elem)) {
            ebml_element_t child;
            err = ebml_read_element_header(handle, &child);
            if (err != MKVTAG_OK) break;

            if (child.id == MKV_ID_SEEK_ID) {
                /* SeekID is stored as binary data representing an element ID */
                uint8_t id_buf[4];
                size_t bytes_read;
                err = ebml_read_binary(handle, &child, id_buf, sizeof(id_buf), &bytes_read);
                if (err == MKVTAG_OK && bytes_read > 0 && bytes_read <= 4) {
                    seek_id = 0;
                    for (size_t i = 0; i < bytes_read; i++) {
                        seek_id = (seek_id << 8) | id_buf[i];
                    }
                }
            } else if (child.id == MKV_ID_SEEK_POSITION) {
                ebml_read_uint(handle, &child, &seek_position);
            }

            ebml_skip_element(handle, &child);
        }

        if (seek_id != 0) {
            int64_t abs_offset = mkv_segment_to_absolute(mkv, seek_position);
            mkv_store_element_offset(mkv, seek_id, abs_offset);
            mkv_cache_element(mkv, seek_id, abs_offset, 0);
        }
    }

    return MKVTAG_OK;
}

/**
 * Scan Segment children to find key elements.
 * Stops at the first Cluster to avoid reading media data.
 */
static int mkv_scan_structure(mkv_file_t *mkv) {
    file_handle_t *handle = mkv->handle;

    int err = file_seek(handle, mkv->segment_data_offset);
    if (err != MKVTAG_OK) return err;

    int64_t segment_end;
    if (mkv->segment_unknown_size) {
        segment_end = file_size(handle);
    } else {
        segment_end = mkv->segment_data_offset + (int64_t)mkv->segment_size;
    }

    while (file_tell(handle) < segment_end) {
        int64_t elem_offset = file_tell(handle);

        ebml_element_t elem;
        err = ebml_read_element_header(handle, &elem);
        if (err != MKVTAG_OK) break;

        mkv_store_element_offset(mkv, elem.id, elem_offset);

        /* Track Void elements for potential tag writing space */
        if (elem.id == EBML_ID_VOID) {
            uint64_t total_size = (uint64_t)(elem.data_offset - elem_offset) + elem.size;
            if (mkv->void_offset < 0 || total_size > mkv->void_size) {
                mkv->void_offset = elem_offset;
                mkv->void_size = total_size;
            }
        }

        /* Stop at first Cluster - no need to scan media data */
        if (elem.id == MKV_ID_CLUSTER) {
            break;
        }

        /* Skip this element */
        err = ebml_skip_element(handle, &elem);
        if (err != MKVTAG_OK) break;
    }

    return MKVTAG_OK;
}

int mkv_parse_structure(mkv_file_t *mkv) {
    if (!mkv || !mkv->handle) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    file_handle_t *handle = mkv->handle;

    /* Find the Segment element (should be right after EBML header) */
    /* Position should already be after EBML header from mkv_parse_header */
    ebml_element_t segment;
    int err = ebml_read_element_header(handle, &segment);
    if (err != MKVTAG_OK) {
        return MKVTAG_ERR_NOT_MKV;
    }

    if (segment.id != MKV_ID_SEGMENT) {
        return MKVTAG_ERR_NOT_MKV;
    }

    mkv->segment_offset = segment.data_offset - segment.id_length - segment.size_length;
    mkv->segment_data_offset = segment.data_offset;
    mkv->segment_size = segment.size;
    mkv->segment_unknown_size = segment.is_unknown_size;

    /* First pass: scan for elements before clusters */
    err = mkv_scan_structure(mkv);
    if (err != MKVTAG_OK) {
        return err;
    }

    /* If we found a SeekHead, use it to find elements after clusters */
    if (mkv->seekhead_offset >= 0) {
        err = file_seek(handle, mkv->seekhead_offset);
        if (err != MKVTAG_OK) return err;

        ebml_element_t seekhead;
        err = ebml_read_element_header(handle, &seekhead);
        if (err == MKVTAG_OK && seekhead.id == MKV_ID_SEEKHEAD) {
            mkv_parse_seekhead(mkv, &seekhead);
        }
    }

    return MKVTAG_OK;
}

int mkv_find_element(mkv_file_t *mkv, const ebml_element_t *parent,
                     uint32_t target_id, ebml_element_t *found) {
    if (!mkv || !parent || !found) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    file_handle_t *handle = mkv->handle;

    while (!ebml_at_element_end(handle, parent)) {
        ebml_element_t elem;
        int err = ebml_read_element_header(handle, &elem);
        if (err != MKVTAG_OK) {
            return err;
        }

        if (elem.id == target_id) {
            *found = elem;
            return MKVTAG_OK;
        }

        /* Skip Clusters efficiently */
        if (elem.id == MKV_ID_CLUSTER && !elem.is_unknown_size) {
            err = ebml_skip_element(handle, &elem);
            if (err != MKVTAG_OK) return err;
            continue;
        }

        err = ebml_skip_element(handle, &elem);
        if (err != MKVTAG_OK) return err;
    }

    return MKVTAG_ERR_TAG_NOT_FOUND;
}

int64_t mkv_segment_to_absolute(const mkv_file_t *mkv, uint64_t relative_position) {
    if (!mkv) return -1;
    return mkv->segment_data_offset + (int64_t)relative_position;
}

uint64_t mkv_absolute_to_segment(const mkv_file_t *mkv, int64_t absolute_offset) {
    if (!mkv) return 0;
    return (uint64_t)(absolute_offset - mkv->segment_data_offset);
}

void mkv_cache_element(mkv_file_t *mkv, uint32_t id, int64_t offset, uint64_t size) {
    if (!mkv || mkv->element_count >= MKV_MAX_CACHED_ELEMENTS) {
        return;
    }

    /* Check for duplicate */
    for (int i = 0; i < mkv->element_count; i++) {
        if (mkv->elements[i].id == id) {
            mkv->elements[i].offset = offset;
            mkv->elements[i].size = size;
            return;
        }
    }

    mkv->elements[mkv->element_count].id = id;
    mkv->elements[mkv->element_count].offset = offset;
    mkv->elements[mkv->element_count].size = size;
    mkv->element_count++;
}

const mkv_element_pos_t *mkv_lookup_element(const mkv_file_t *mkv, uint32_t id) {
    if (!mkv) return NULL;

    for (int i = 0; i < mkv->element_count; i++) {
        if (mkv->elements[i].id == id) {
            return &mkv->elements[i];
        }
    }

    return NULL;
}
