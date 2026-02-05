/*
 * ebml_ids.h - EBML and Matroska element ID constants
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#ifndef EBML_IDS_H
#define EBML_IDS_H

#include <stdint.h>

/*
 * EBML Header Elements
 */
#define EBML_ID_EBML                0x1A45DFA3
#define EBML_ID_VERSION             0x4286
#define EBML_ID_READ_VERSION        0x42F7
#define EBML_ID_MAX_ID_LENGTH       0x42F2
#define EBML_ID_MAX_SIZE_LENGTH     0x42F3
#define EBML_ID_DOCTYPE             0x4282
#define EBML_ID_DOCTYPE_VERSION     0x4287
#define EBML_ID_DOCTYPE_READ_VER    0x4285

/*
 * Global Elements
 */
#define EBML_ID_VOID                0xEC
#define EBML_ID_CRC32               0xBF

/*
 * Matroska Segment
 */
#define MKV_ID_SEGMENT              0x18538067

/*
 * SeekHead (Meta Seek Information)
 */
#define MKV_ID_SEEKHEAD             0x114D9B74
#define MKV_ID_SEEK                 0x4DBB
#define MKV_ID_SEEK_ID              0x53AB
#define MKV_ID_SEEK_POSITION        0x53AC

/*
 * Segment Information
 */
#define MKV_ID_INFO                 0x1549A966
#define MKV_ID_SEGMENT_UID          0x73A4
#define MKV_ID_SEGMENT_FILENAME     0x7384
#define MKV_ID_TIMECODE_SCALE       0x2AD7B1
#define MKV_ID_DURATION             0x4489
#define MKV_ID_DATE_UTC             0x4461
#define MKV_ID_TITLE                0x7BA9
#define MKV_ID_MUXING_APP           0x4D80
#define MKV_ID_WRITING_APP          0x5741

/*
 * Cluster (Media Data)
 */
#define MKV_ID_CLUSTER              0x1F43B675
#define MKV_ID_TIMECODE             0xE7
#define MKV_ID_SIMPLE_BLOCK         0xA3
#define MKV_ID_BLOCK_GROUP          0xA0
#define MKV_ID_BLOCK                0xA1

/*
 * Tracks
 */
#define MKV_ID_TRACKS               0x1654AE6B
#define MKV_ID_TRACK_ENTRY          0xAE
#define MKV_ID_TRACK_NUMBER         0xD7
#define MKV_ID_TRACK_UID            0x73C5
#define MKV_ID_TRACK_TYPE           0x83
#define MKV_ID_FLAG_ENABLED         0xB9
#define MKV_ID_FLAG_DEFAULT         0x88
#define MKV_ID_FLAG_FORCED          0x55AA
#define MKV_ID_FLAG_LACING          0x9C
#define MKV_ID_MIN_CACHE            0x6DE7
#define MKV_ID_MAX_CACHE            0x6DF8
#define MKV_ID_DEFAULT_DURATION     0x23E383
#define MKV_ID_TRACK_TIMECODE_SCALE 0x23314F
#define MKV_ID_NAME                 0x536E
#define MKV_ID_LANGUAGE             0x22B59C
#define MKV_ID_CODEC_ID             0x86
#define MKV_ID_CODEC_PRIVATE        0x63A2
#define MKV_ID_CODEC_NAME           0x258688
#define MKV_ID_ATTACHMENT_LINK      0x7446

/*
 * Video
 */
#define MKV_ID_VIDEO                0xE0
#define MKV_ID_PIXEL_WIDTH          0xB0
#define MKV_ID_PIXEL_HEIGHT         0xBA
#define MKV_ID_DISPLAY_WIDTH        0x54B0
#define MKV_ID_DISPLAY_HEIGHT       0x54BA
#define MKV_ID_DISPLAY_UNIT         0x54B2

/*
 * Audio
 */
#define MKV_ID_AUDIO                0xE1
#define MKV_ID_SAMPLING_FREQ        0xB5
#define MKV_ID_OUTPUT_SAMPLING_FREQ 0x78B5
#define MKV_ID_CHANNELS             0x9F
#define MKV_ID_BIT_DEPTH            0x6264

/*
 * Cues (Index)
 */
#define MKV_ID_CUES                 0x1C53BB6B
#define MKV_ID_CUE_POINT            0xBB
#define MKV_ID_CUE_TIME             0xB3
#define MKV_ID_CUE_TRACK_POSITIONS  0xB7
#define MKV_ID_CUE_TRACK            0xF7
#define MKV_ID_CUE_CLUSTER_POSITION 0xF1
#define MKV_ID_CUE_BLOCK_NUMBER     0x5378

/*
 * Attachments
 */
#define MKV_ID_ATTACHMENTS          0x1941A469
#define MKV_ID_ATTACHED_FILE        0x61A7
#define MKV_ID_FILE_DESCRIPTION     0x467E
#define MKV_ID_FILE_NAME            0x466E
#define MKV_ID_FILE_MIME_TYPE       0x4660
#define MKV_ID_FILE_DATA            0x465C
#define MKV_ID_FILE_UID             0x46AE

/*
 * Chapters
 */
#define MKV_ID_CHAPTERS             0x1043A770
#define MKV_ID_EDITION_ENTRY        0x45B9
#define MKV_ID_EDITION_UID          0x45BC
#define MKV_ID_EDITION_FLAG_HIDDEN  0x45BD
#define MKV_ID_EDITION_FLAG_DEFAULT 0x45DB
#define MKV_ID_EDITION_FLAG_ORDERED 0x45DD
#define MKV_ID_CHAPTER_ATOM         0xB6
#define MKV_ID_CHAPTER_UID          0x73C4
#define MKV_ID_CHAPTER_TIME_START   0x91
#define MKV_ID_CHAPTER_TIME_END     0x92
#define MKV_ID_CHAPTER_FLAG_HIDDEN  0x98
#define MKV_ID_CHAPTER_FLAG_ENABLED 0x4598
#define MKV_ID_CHAPTER_DISPLAY      0x80
#define MKV_ID_CHAP_STRING          0x85
#define MKV_ID_CHAP_LANGUAGE        0x437C
#define MKV_ID_CHAP_COUNTRY         0x437E

/*
 * Tags - The main focus of this library
 */
#define MKV_ID_TAGS                 0x1254C367
#define MKV_ID_TAG                  0x7373
#define MKV_ID_TARGETS              0x63C0
#define MKV_ID_TARGET_TYPE_VALUE    0x68CA
#define MKV_ID_TARGET_TYPE          0x63CA
#define MKV_ID_TAG_TRACK_UID        0x63C5
#define MKV_ID_TAG_EDITION_UID      0x63C9
#define MKV_ID_TAG_CHAPTER_UID      0x63C4
#define MKV_ID_TAG_ATTACHMENT_UID   0x63C6
#define MKV_ID_SIMPLE_TAG           0x67C8
#define MKV_ID_TAG_NAME             0x45A3
#define MKV_ID_TAG_LANGUAGE         0x447A
#define MKV_ID_TAG_LANGUAGE_BCP47   0x447B
#define MKV_ID_TAG_DEFAULT          0x4484
#define MKV_ID_TAG_STRING           0x4487
#define MKV_ID_TAG_BINARY           0x4485

/*
 * Helper macros for element ID byte-count ranges.
 * Element IDs include the VINT marker bits, so the ranges
 * correspond to raw byte counts of the ID value.
 */
#define EBML_ID_SIZE_1_BYTE_MAX     0xFF
#define EBML_ID_SIZE_2_BYTE_MAX     0xFFFF
#define EBML_ID_SIZE_3_BYTE_MAX     0xFFFFFF
#define EBML_ID_SIZE_4_BYTE_MAX     0xFFFFFFFF

/**
 * Get the number of bytes needed to encode an element ID.
 * Element IDs include the VINT marker bits in their value,
 * so this is simply the byte width of the raw value.
 */
static inline int ebml_id_size(uint32_t id) {
    if (id <= 0xFF) return 1;
    if (id <= 0xFFFF) return 2;
    if (id <= 0xFFFFFF) return 3;
    return 4;
}

/**
 * Check if an element ID is a master element (container).
 */
static inline int ebml_is_master_element(uint32_t id) {
    switch (id) {
        case EBML_ID_EBML:
        case MKV_ID_SEGMENT:
        case MKV_ID_SEEKHEAD:
        case MKV_ID_SEEK:
        case MKV_ID_INFO:
        case MKV_ID_CLUSTER:
        case MKV_ID_TRACKS:
        case MKV_ID_TRACK_ENTRY:
        case MKV_ID_VIDEO:
        case MKV_ID_AUDIO:
        case MKV_ID_CUES:
        case MKV_ID_CUE_POINT:
        case MKV_ID_CUE_TRACK_POSITIONS:
        case MKV_ID_ATTACHMENTS:
        case MKV_ID_ATTACHED_FILE:
        case MKV_ID_CHAPTERS:
        case MKV_ID_EDITION_ENTRY:
        case MKV_ID_CHAPTER_ATOM:
        case MKV_ID_CHAPTER_DISPLAY:
        case MKV_ID_TAGS:
        case MKV_ID_TAG:
        case MKV_ID_TARGETS:
        case MKV_ID_SIMPLE_TAG:
        case MKV_ID_BLOCK_GROUP:
            return 1;
        default:
            return 0;
    }
}

#endif /* EBML_IDS_H */
