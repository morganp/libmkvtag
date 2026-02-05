/*
 * mkv_seekhead.h - SeekHead parsing and updating
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef MKV_SEEKHEAD_H
#define MKV_SEEKHEAD_H

#include <stdint.h>
#include "../util/buffer.h"

/* Forward declaration */
typedef struct mkv_file mkv_file_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Build a SeekHead entry (Seek element) for a given element.
 *
 * @param buf Output buffer to append to
 * @param element_id The element ID to reference
 * @param segment_relative_pos The element's position relative to Segment data start
 * @return 0 on success, negative error code on failure
 */
int mkv_seekhead_build_entry(dyn_buffer_t *buf, uint32_t element_id,
                             uint64_t segment_relative_pos);

/**
 * Update the SeekHead to include a Tags element at the given position.
 * This attempts to update the existing SeekHead in-place.
 *
 * @param mkv The MKV file structure
 * @param tags_offset Absolute file offset of the Tags element
 * @return 0 on success, negative error code on failure
 */
int mkv_seekhead_update_tags(mkv_file_t *mkv, int64_t tags_offset);

#ifdef __cplusplus
}
#endif

#endif /* MKV_SEEKHEAD_H */
