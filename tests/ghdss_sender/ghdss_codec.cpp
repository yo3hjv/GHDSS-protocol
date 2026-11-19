#include "ghdss_codec.h"
#include <string.h>
#include <ctype.h>

// ============================================================================
// Canonical CRC-16-CCITT (Appendix I.3)
// Polynomial: 0x1021, Initial: 0xFFFF, MSB First
// ============================================================================
uint16_t ghdss_crc16(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
            } else {
                crc = (crc << 1) & 0xFFFF;
            }
        }
    }
    return crc;
}

// ============================================================================
// Base40 Radix-40 Encoding & Decoding (Appendix I.1)
// 0: SPACE, 1..10: '0'..'9', 11..36: 'A'..'Z', 37: '/', 38..39: RESERVED
// ============================================================================
static uint8_t char_to_base40(char c) {
    if (c >= '0' && c <= '9') {
        return (uint8_t)(c - '0' + 1);
    }
    if (c >= 'A' && c <= 'Z') {
        return (uint8_t)(c - 'A' + 11);
    }
    if (c >= 'a' && c <= 'z') {
        return (uint8_t)(c - 'a' + 11);
    }
    if (c == '/') {
        return 37;
    }
    return 0; // SPACE or invalid
}

static char base40_to_char(uint8_t val) {
    if (val == 0) return ' ';
    if (val >= 1 && val <= 10) return (char)('0' + (val - 1));
    if (val >= 11 && val <= 36) return (char)('A' + (val - 11));
    if (val == 37) return '/';
    return '?';
}

void ghdss_encode_base40(const char *callsign, uint8_t *out_bytes) {
    char padded[9];
    size_t len = strlen(callsign);
    for (int i = 0; i < 9; i++) {
        if ((size_t)i < len) {
            padded[i] = (char)toupper((unsigned char)callsign[i]);
        } else {
            padded[i] = ' ';
        }
    }

    for (int triplet = 0; triplet < 3; triplet++) {
        uint8_t c0 = char_to_base40(padded[triplet * 3 + 0]);
        uint8_t c1 = char_to_base40(padded[triplet * 3 + 1]);
        uint8_t c2 = char_to_base40(padded[triplet * 3 + 2]);

        uint16_t word = ((uint16_t)c0 * 1600) + ((uint16_t)c1 * 40) + (uint16_t)c2;
        out_bytes[triplet * 2 + 0] = (uint8_t)((word >> 8) & 0xFF);
        out_bytes[triplet * 2 + 1] = (uint8_t)(word & 0xFF);
    }
}

void ghdss_decode_base40(const uint8_t *in_bytes, char *out_callsign) {
    int idx = 0;
    for (int triplet = 0; triplet < 3; triplet++) {
        uint16_t word = ((uint16_t)in_bytes[triplet * 2 + 0] << 8) | in_bytes[triplet * 2 + 1];
        uint8_t c2 = word % 40; word /= 40;
        uint8_t c1 = word % 40; word /= 40;
        uint8_t c0 = word % 40;

        out_callsign[idx++] = base40_to_char(c0);
        out_callsign[idx++] = base40_to_char(c1);
        out_callsign[idx++] = base40_to_char(c2);
    }
    out_callsign[idx] = '\0';

    // Right-trim trailing spaces
    for (int i = idx - 1; i >= 0; i--) {
        if (out_callsign[i] == ' ') {
            out_callsign[i] = '\0';
        } else {
            break;
        }
    }
}

// ============================================================================
// Packet Builders
// ============================================================================

size_t ghdss_build_alert_packet(uint8_t *buffer, size_t max_len,
                                uint16_t msg_id,
                                const char *callsign, uint8_t ssid, uint8_t role,
                                uint8_t ttl,
                                int32_t lat_scaled, int32_t lon_scaled,
                                uint32_t utc_time,
                                uint8_t hazard_code,
                                const char *detail_text) {
    size_t text_len = detail_text ? strlen(detail_text) : 0;
    if (text_len > GHDSS_MAX_TEXT_LEN) {
        text_len = GHDSS_MAX_TEXT_LEN;
    }

    // Packet length = 2 (PID) + 12 (Header) + 0 (Dest=ALL) + 15 (Alert base payload) + text_len + 2 (CRC)
    size_t total_len = 2 + 12 + 15 + text_len + 2;
    if (total_len > max_len) return 0;

    size_t pos = 0;

    // 1. Protocol ID
    buffer[pos++] = GHDSS_PROTOCOL_ID_BYTE0; // 0x47 'G'
    buffer[pos++] = GHDSS_PROTOCOL_ID_BYTE1; // 0x48 'H'

    // 2. Header Byte 2: Version=1 (bits 7..4), Priority=DISTRESS (bits 3..2), Dest=ALL (bits 1..0)
    buffer[pos++] = (uint8_t)((GHDSS_PROTOCOL_VERSION << 4) | (GHDSS_PRIORITY_DISTRESS << 2) | GHDSS_DEST_ALL_STATIONS);

    // 3. Header Byte 3: Type=ALERT (0x00), Flags=ACK_REQUIRED | RELAY_ALLOWED
    buffer[pos++] = (uint8_t)((GHDSS_MSG_ALERT << 4) | GHDSS_FLAG_ACK_REQUIRED | GHDSS_FLAG_RELAY_ALLOWED);

    // 4. Header Byte 4: TTL (bits 7..5), Options=0
    buffer[pos++] = (uint8_t)((ttl & 0x07) << 5);

    // 5. Message ID (Big Endian)
    buffer[pos++] = (uint8_t)((msg_id >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(msg_id & 0xFF);

    // 6. Originator Station ID (7 bytes: 6B Base40 + 1B SSID/Role)
    ghdss_encode_base40(callsign, &buffer[pos]);
    pos += 6;
    buffer[pos++] = (uint8_t)(((ssid & 0x1F) << 3) | (role & 0x07));

    // 7. ALERT Payload (15 bytes base):
    // - Hazard Code (Byte 14): bits 7..3 Hazard, bits 2..0 Quality_hi
    buffer[pos++] = (uint8_t)((hazard_code & 0x1F) << 2);

    // - Quality & Age (Byte 15): Quality_lo=3 (GNSS), Age=0, Flags=0 -> 0xC0
    buffer[pos++] = 0xC0;

    // - Latitude (int32_t, Big Endian)
    buffer[pos++] = (uint8_t)((lat_scaled >> 24) & 0xFF);
    buffer[pos++] = (uint8_t)((lat_scaled >> 16) & 0xFF);
    buffer[pos++] = (uint8_t)((lat_scaled >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(lat_scaled & 0xFF);

    // - Longitude (int32_t, Big Endian)
    buffer[pos++] = (uint8_t)((lon_scaled >> 24) & 0xFF);
    buffer[pos++] = (uint8_t)((lon_scaled >> 16) & 0xFF);
    buffer[pos++] = (uint8_t)((lon_scaled >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(lon_scaled & 0xFF);

    // - UTC Timestamp (uint32_t, Big Endian)
    buffer[pos++] = (uint8_t)((utc_time >> 24) & 0xFF);
    buffer[pos++] = (uint8_t)((utc_time >> 16) & 0xFF);
    buffer[pos++] = (uint8_t)((utc_time >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(utc_time & 0xFF);

    // - Text Length (1 byte) + Detail Text
    buffer[pos++] = (uint8_t)text_len;
    if (text_len > 0) {
        memcpy(&buffer[pos], detail_text, text_len);
        pos += text_len;
    }

    // 8. CRC-16-CCITT (calculated over all preceding bytes)
    uint16_t crc = ghdss_crc16(buffer, pos);
    buffer[pos++] = (uint8_t)((crc >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(crc & 0xFF);

    return pos;
}

size_t ghdss_build_ack_packet(uint8_t *buffer, size_t max_len,
                              uint16_t msg_id,
                              const char *my_callsign, uint8_t my_ssid, uint8_t my_role,
                              const char *target_callsign, uint8_t target_ssid, uint8_t target_role,
                              uint16_t ref_msg_id,
                              uint8_t result_code) {
    // Total len = 2 (PID) + 12 (Header) + 0 (Dest=ALL) + 10 (ACK payload) + 2 (CRC) = 26 bytes
    size_t total_len = 26;
    if (total_len > max_len) return 0;

    size_t pos = 0;

    // 1. Protocol ID
    buffer[pos++] = GHDSS_PROTOCOL_ID_BYTE0;
    buffer[pos++] = GHDSS_PROTOCOL_ID_BYTE1;

    // 2. Header Byte 2: Version=1, Priority=DISTRESS (same priority class as alert), Dest=ALL
    buffer[pos++] = (uint8_t)((GHDSS_PROTOCOL_VERSION << 4) | (GHDSS_PRIORITY_DISTRESS << 2) | GHDSS_DEST_ALL_STATIONS);

    // 3. Header Byte 3: Type=ACK (0x01 << 4 = 0x10), Flags=0
    buffer[pos++] = (uint8_t)(GHDSS_MSG_ACK << 4);

    // 4. Header Byte 4: TTL=1 (ACK is typically single hop or local responder)
    buffer[pos++] = (uint8_t)(1 << 5);

    // 5. Message ID
    buffer[pos++] = (uint8_t)((msg_id >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(msg_id & 0xFF);

    // 6. Originator (Responder) ID (7 bytes)
    ghdss_encode_base40(my_callsign, &buffer[pos]);
    pos += 6;
    buffer[pos++] = (uint8_t)(((my_ssid & 0x1F) << 3) | (my_role & 0x07));

    // 7. ACK Payload:
    // - Ref Originator ID (7 bytes: 6B Base40 + 1B SSID/Role)
    ghdss_encode_base40(target_callsign, &buffer[pos]);
    pos += 6;
    buffer[pos++] = (uint8_t)(((target_ssid & 0x1F) << 3) | (target_role & 0x07));

    // - Ref Message ID (2 bytes, Big Endian)
    buffer[pos++] = (uint8_t)((ref_msg_id >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(ref_msg_id & 0xFF);

    // - Result Code (1 byte: 0 = Accepted/Valid)
    buffer[pos++] = result_code;

    // 8. CRC-16-CCITT
    uint16_t crc = ghdss_crc16(buffer, pos);
    buffer[pos++] = (uint8_t)((crc >> 8) & 0xFF);
    buffer[pos++] = (uint8_t)(crc & 0xFF);

    return pos;
}

// ============================================================================
// Packet Parser & Validator
// ============================================================================

bool ghdss_parse_packet(const uint8_t *buffer, size_t length, GhdssParsedPacket *out) {
    if (!buffer || !out || length < 16) return false;
    memset(out, 0, sizeof(GhdssParsedPacket));

    // Check Protocol ID "GH"
    if (buffer[0] != GHDSS_PROTOCOL_ID_BYTE0 || buffer[1] != GHDSS_PROTOCOL_ID_BYTE1) {
        return false;
    }

    // Verify CRC-16
    size_t payload_and_hdr_len = length - 2;
    out->received_crc = ((uint16_t)buffer[length - 2] << 8) | buffer[length - 1];
    out->calculated_crc = ghdss_crc16(buffer, payload_and_hdr_len);
    out->crc_valid = (out->received_crc == out->calculated_crc);
    if (!out->crc_valid) {
        return false;
    }

    // Parse Header
    out->version   = (buffer[2] >> 4) & 0x0F;
    out->priority  = (buffer[2] >> 2) & 0x03;
    out->dest_type = buffer[2] & 0x03;

    out->msg_type  = (buffer[3] >> 4) & 0x0F;
    out->flags     = buffer[3] & 0x0F;

    out->ttl       = (buffer[4] >> 5) & 0x07;

    out->msg_id    = ((uint16_t)buffer[5] << 8) | buffer[6];

    // Originator Station ID
    ghdss_decode_base40(&buffer[7], out->originator.callsign);
    out->originator.ssid = (buffer[13] >> 3) & 0x1F;
    out->originator.role = buffer[13] & 0x07;

    size_t pos = 14;

    // Skip addressing field if present
    if (out->dest_type == GHDSS_DEST_INDIVIDUAL) pos += 7;
    else if (out->dest_type == GHDSS_DEST_GROUP) pos += 2;
    else if (out->dest_type == GHDSS_DEST_GRID) pos += 4;

    if (pos >= payload_and_hdr_len) return true;

    // Parse Payload by message type
    if (out->msg_type == GHDSS_MSG_ALERT && pos + 15 <= payload_and_hdr_len) {
        out->hazard_code = (buffer[pos] >> 2) & 0x1F;
        pos += 2; // skip hazard byte + quality/age byte

        out->lat_scaled = ((int32_t)buffer[pos] << 24) |
                          ((int32_t)buffer[pos + 1] << 16) |
                          ((int32_t)buffer[pos + 2] << 8) |
                          ((int32_t)buffer[pos + 3]);
        pos += 4;

        out->lon_scaled = ((int32_t)buffer[pos] << 24) |
                          ((int32_t)buffer[pos + 1] << 16) |
                          ((int32_t)buffer[pos + 2] << 8) |
                          ((int32_t)buffer[pos + 3]);
        pos += 4;

        out->utc_timestamp = ((uint32_t)buffer[pos] << 24) |
                             ((uint32_t)buffer[pos + 1] << 16) |
                             ((uint32_t)buffer[pos + 2] << 8) |
                             ((uint32_t)buffer[pos + 3]);
        pos += 4;

        out->text_length = buffer[pos++];
        if (out->text_length > 0 && pos + out->text_length <= payload_and_hdr_len) {
            size_t copy_len = (out->text_length < GHDSS_MAX_TEXT_LEN) ? out->text_length : GHDSS_MAX_TEXT_LEN;
            memcpy(out->detail_text, &buffer[pos], copy_len);
            out->detail_text[copy_len] = '\0';
        }
    } else if (out->msg_type == GHDSS_MSG_ACK && pos + 10 <= payload_and_hdr_len) {
        ghdss_decode_base40(&buffer[pos], out->ref_originator.callsign);
        pos += 6;
        out->ref_originator.ssid = (buffer[pos] >> 3) & 0x1F;
        out->ref_originator.role = buffer[pos] & 0x07;
        pos++;

        out->ref_msg_id = ((uint16_t)buffer[pos] << 8) | buffer[pos + 1];
        pos += 2;

        out->ack_result_code = buffer[pos++];
    }

    return true;
}
