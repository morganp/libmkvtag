/*
 * ebml_vint.c - EBML variable-length integer encoding/decoding
 *
 * Copyright (c) 2024
 * SPDX-License-Identifier: MIT
 */

#include "ebml_vint.h"
#include "../../include/mkvtag/mkvtag_error.h"

int ebml_vint_size(uint64_t value) {
    if (value <= EBML_VINT_MAX_1) return 1;
    if (value <= EBML_VINT_MAX_2) return 2;
    if (value <= EBML_VINT_MAX_3) return 3;
    if (value <= EBML_VINT_MAX_4) return 4;
    if (value <= EBML_VINT_MAX_5) return 5;
    if (value <= EBML_VINT_MAX_6) return 6;
    if (value <= EBML_VINT_MAX_7) return 7;
    if (value <= EBML_VINT_MAX_8) return 8;
    return 0; /* Value too large */
}

int ebml_vint_length(uint8_t first_byte) {
    if (first_byte & 0x80) return 1;       /* 1xxxxxxx */
    if (first_byte & 0x40) return 2;       /* 01xxxxxx */
    if (first_byte & 0x20) return 3;       /* 001xxxxx */
    if (first_byte & 0x10) return 4;       /* 0001xxxx */
    if (first_byte & 0x08) return 5;       /* 00001xxx */
    if (first_byte & 0x04) return 6;       /* 000001xx */
    if (first_byte & 0x02) return 7;       /* 0000001x */
    if (first_byte & 0x01) return 8;       /* 00000001 */
    return 0; /* Invalid: 00000000 */
}

int ebml_vint_decode(const uint8_t *data, size_t size,
                     uint64_t *value, int *bytes_read) {
    if (!data || !value || !bytes_read) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (size == 0) {
        return MKVTAG_ERR_TRUNCATED;
    }

    int length = ebml_vint_length(data[0]);
    if (length == 0) {
        return MKVTAG_ERR_INVALID_VINT;
    }

    if ((size_t)length > size) {
        return MKVTAG_ERR_TRUNCATED;
    }

    /* Extract value by masking out the length indicator bits */
    uint64_t result = data[0];

    /* Mask off the length indicator bit */
    uint8_t mask = 0xFF >> length;
    result &= mask;

    /* Add remaining bytes */
    for (int i = 1; i < length; i++) {
        result = (result << 8) | data[i];
    }

    *value = result;
    *bytes_read = length;
    return MKVTAG_OK;
}

int ebml_id_decode(const uint8_t *data, size_t size,
                   uint32_t *id, int *bytes_read) {
    if (!data || !id || !bytes_read) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (size == 0) {
        return MKVTAG_ERR_TRUNCATED;
    }

    int length = ebml_vint_length(data[0]);
    if (length == 0) {
        return MKVTAG_ERR_INVALID_VINT;
    }

    if (length > 4) {
        /* Element IDs are at most 4 bytes */
        return MKVTAG_ERR_VINT_OVERFLOW;
    }

    if ((size_t)length > size) {
        return MKVTAG_ERR_TRUNCATED;
    }

    /* For element IDs, we keep the VINT marker bits */
    uint32_t result = 0;
    for (int i = 0; i < length; i++) {
        result = (result << 8) | data[i];
    }

    *id = result;
    *bytes_read = length;
    return MKVTAG_OK;
}

int ebml_vint_encode(uint64_t value, uint8_t *data, int *bytes_written) {
    if (!data || !bytes_written) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    int length = ebml_vint_size(value);
    if (length == 0) {
        return MKVTAG_ERR_VINT_OVERFLOW;
    }

    *bytes_written = length;
    return ebml_vint_encode_fixed(value, length, data);
}

int ebml_vint_encode_fixed(uint64_t value, int length, uint8_t *data) {
    if (!data) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    if (length < 1 || length > 8) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /* Check that value fits in the requested length */
    uint64_t max_value;
    switch (length) {
        case 1: max_value = EBML_VINT_MAX_1; break;
        case 2: max_value = EBML_VINT_MAX_2; break;
        case 3: max_value = EBML_VINT_MAX_3; break;
        case 4: max_value = EBML_VINT_MAX_4; break;
        case 5: max_value = EBML_VINT_MAX_5; break;
        case 6: max_value = EBML_VINT_MAX_6; break;
        case 7: max_value = EBML_VINT_MAX_7; break;
        case 8: max_value = EBML_VINT_MAX_8; break;
        default: return MKVTAG_ERR_INVALID_ARG;
    }

    if (value > max_value) {
        return MKVTAG_ERR_VINT_OVERFLOW;
    }

    /* Encode bytes from right to left */
    for (int i = length - 1; i >= 0; i--) {
        data[i] = (uint8_t)(value & 0xFF);
        value >>= 8;
    }

    /* Set the VINT length marker bit in the first byte */
    data[0] |= (0x80 >> (length - 1));

    return MKVTAG_OK;
}

int ebml_id_encode(uint32_t id, uint8_t *data, int *bytes_written) {
    if (!data || !bytes_written) {
        return MKVTAG_ERR_INVALID_ARG;
    }

    /*
     * Element IDs include the VINT marker bits in their value.
     * The length is determined by how many bytes the raw value needs.
     * For example: 0xEC (1 byte), 0x4286 (2 bytes), 0x1A45DFA3 (4 bytes).
     */
    int length;
    if (id == 0) {
        return MKVTAG_ERR_INVALID_VINT;
    } else if (id <= 0xFF) {
        length = 1;
    } else if (id <= 0xFFFF) {
        length = 2;
    } else if (id <= 0xFFFFFF) {
        length = 3;
    } else {
        length = 4;
    }

    /* Encode bytes from right to left */
    for (int i = length - 1; i >= 0; i--) {
        data[i] = (uint8_t)(id & 0xFF);
        id >>= 8;
    }

    *bytes_written = length;
    return MKVTAG_OK;
}

int ebml_vint_is_unknown(uint64_t value, int length) {
    switch (length) {
        case 1: return value == (EBML_VINT_UNKNOWN_1 & 0x7F);
        case 2: return value == (EBML_VINT_UNKNOWN_2 & 0x3FFF);
        case 3: return value == (EBML_VINT_UNKNOWN_3 & 0x1FFFFF);
        case 4: return value == (EBML_VINT_UNKNOWN_4 & 0x0FFFFFFF);
        case 5: return value == (EBML_VINT_UNKNOWN_5 & 0x07FFFFFFFFULL);
        case 6: return value == (EBML_VINT_UNKNOWN_6 & 0x03FFFFFFFFFFULL);
        case 7: return value == (EBML_VINT_UNKNOWN_7 & 0x01FFFFFFFFFFFFULL);
        case 8: return value == (EBML_VINT_UNKNOWN_8 & 0x00FFFFFFFFFFFFFFULL);
        default: return 0;
    }
}
