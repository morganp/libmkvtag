/*
 * mkv_seekhead.c - SeekHead parsing and updating
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "mkv_seekhead.h"
#include "mkv_parser.h"
#include "../ebml/ebml_ids.h"
#include "../ebml/ebml_vint.h"
#include "../ebml/ebml_reader.h"
#include "../ebml/ebml_writer.h"
#include "../include/mkvtag/mkvtag_error.h"

#include <string.h>

int mkv_seekhead_build_entry(dyn_buffer_t *buf, uint32_t element_id,
                             uint64_t segment_relative_pos) {
    if (!buf) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /*
     * Build a Seek element:
     *   Seek [4DBB] {
     *     SeekID [53AB] { <element_id bytes> }
     *     SeekPosition [53AC] { <position as uint> }
     *   }
     */

    /* First, build the inner content */
    dyn_buffer_t inner;
    buffer_init(&inner);

    /* SeekID element */
    uint8_t id_bytes[4];
    int id_len;
    int err = ebml_id_encode(element_id, id_bytes, &id_len);
    if (err != MKVTAG_OK) {
        buffer_free(&inner);
        return err;
    }

    err = ebml_write_binary_element(&inner, MKV_ID_SEEK_ID, id_bytes, id_len);
    if (err != MKVTAG_OK) {
        buffer_free(&inner);
        return err;
    }

    /* SeekPosition element */
    err = ebml_write_uint_element(&inner, MKV_ID_SEEK_POSITION, segment_relative_pos);
    if (err != MKVTAG_OK) {
        buffer_free(&inner);
        return err;
    }

    /* Now wrap in Seek master element */
    err = ebml_write_master_element_header(buf, MKV_ID_SEEK, inner.size);
    if (err != MKVTAG_OK) {
        buffer_free(&inner);
        return err;
    }

    err = buffer_append(buf, inner.data, inner.size);
    buffer_free(&inner);
    return err;
}

int mkv_seekhead_update_tags(mkv_file_t *mkv, int64_t tags_offset) {
    if (!mkv || !mkv->handle) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* If there's no SeekHead, we can't update it */
    if (mkv->seekhead_offset < 0) {
        return MKVTAG_OK;
    }

    /*
     * Strategy: Read existing SeekHead, find Tags entry, update or add it.
     *
     * For simplicity, we try to update the Tags seek position in-place
     * if a Tags entry already exists. If not, we skip the update since
     * adding a new entry could change the SeekHead size.
     */

    file_handle_t *handle = mkv->handle;

    int err = file_seek(handle, mkv->seekhead_offset);
    if (err != MKVTAG_OK) return err;

    ebml_element_t seekhead;
    err = ebml_read_element_header(handle, &seekhead);
    if (err != MKVTAG_OK || seekhead.id != MKV_ID_SEEKHEAD) {
        return MKVTAG_OK; /* Silently fail */
    }

    /* Scan for an existing Tags seek entry */
    while (!ebml_at_element_end(handle, &seekhead)) {
        ebml_element_t seek_elem;
        err = ebml_read_element_header(handle, &seek_elem);
        if (err != MKVTAG_OK) break;

        if (seek_elem.id != MKV_ID_SEEK) {
            ebml_skip_element(handle, &seek_elem);
            continue;
        }

        /* Parse Seek entry looking for Tags ID */
        uint32_t seek_id = 0;
        int64_t position_data_offset = -1;
        int position_data_size = 0;

        while (!ebml_at_element_end(handle, &seek_elem)) {
            ebml_element_t child;
            err = ebml_read_element_header(handle, &child);
            if (err != MKVTAG_OK) break;

            if (child.id == MKV_ID_SEEK_ID) {
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
                position_data_offset = child.data_offset;
                position_data_size = (int)child.size;
            }

            ebml_skip_element(handle, &child);
        }

        if (seek_id == MKV_ID_TAGS && position_data_offset >= 0 && position_data_size > 0) {
            /* Found Tags entry - update position in-place */
            uint64_t new_pos = mkv_absolute_to_segment(mkv, tags_offset);

            /* Encode position as big-endian uint with same byte count */
            uint8_t pos_buf[8];
            memset(pos_buf, 0, sizeof(pos_buf));
            uint64_t val = new_pos;
            for (int i = position_data_size - 1; i >= 0; i--) {
                pos_buf[i] = (uint8_t)(val & 0xFF);
                val >>= 8;
            }

            /* Check if value fits */
            if (val != 0) {
                /* Position too large for existing field size */
                return MKVTAG_OK; /* Silently skip */
            }

            err = file_seek(handle, position_data_offset);
            if (err != MKVTAG_OK) return err;

            err = file_write(handle, pos_buf, position_data_size);
            return err;
        }
    }

    /* No existing Tags entry found - can't add one without risking size change */
    return MKVTAG_OK;
}
