#ifndef GHDSS_CODEC_H
#define GHDSS_CODEC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// GHDSS Protocol Constants (Layer 3)
// ============================================================================
#define GHDSS_PROTOCOL_ID_BYTE0    0x47  // 'G'
#define GHDSS_PROTOCOL_ID_BYTE1    0x48  // 'H'
#define GHDSS_PROTOCOL_VERSION     1

// Priority Levels (Byte 2, bits 3..2)
#define GHDSS_PRIORITY_ROUTINE     0
#define GHDSS_PRIORITY_SAFETY      1
#define GHDSS_PRIORITY_URGENCY     2
#define GHDSS_PRIORITY_DISTRESS    3

// Destination Types (Byte 2, bits 1..0)
#define GHDSS_DEST_ALL_STATIONS    0
#define GHDSS_DEST_INDIVIDUAL      1
#define GHDSS_DEST_GROUP           2
#define GHDSS_DEST_GRID            3

// Message Types (Byte 3, bits 7..4)
#define GHDSS_MSG_ALERT            0
#define GHDSS_MSG_ACK              1
#define GHDSS_MSG_OP_RESPONSE      2
#define GHDSS_MSG_POSITION         3
#define GHDSS_MSG_TELEMETRY        4
#define GHDSS_MSG_TEST             5
#define GHDSS_MSG_CONTROL_COMMAND  6

// Flags (Byte 3, bits 3..0)
#define GHDSS_FLAG_ACK_REQUIRED    0x08
#define GHDSS_FLAG_RELAY_ALLOWED   0x04
#define GHDSS_FLAG_AUTHENTICATED   0x02
#define GHDSS_FLAG_ENCRYPTED       0x01

// Station Roles (Byte 13, bits 2..0)
#define GHDSS_ROLE_OTHER           0
#define GHDSS_ROLE_SAR             1
#define GHDSS_ROLE_EMCOMM          2
#define GHDSS_ROLE_GATEWAY         3
#define GHDSS_ROLE_CONTROL         4

// Hazard Codes (Appendix A)
#define GHDSS_HAZARD_UNSPECIFIED   0
#define GHDSS_HAZARD_GENERAL       1
#define GHDSS_HAZARD_FIRE          2
#define GHDSS_HAZARD_FLOOD         3
#define GHDSS_HAZARD_EARTHQUAKE    4
#define GHDSS_HAZARD_MEDICAL       7
#define GHDSS_HAZARD_LOST_PERSON   10

// Maximum Constraints
#define GHDSS_MAX_PACKET_LEN       170
#define GHDSS_MAX_TEXT_LEN         128

// ============================================================================
// Data Structures
// ============================================================================
typedef struct {
    char callsign[10];      // Up to 9 chars + null terminator
    uint8_t ssid;           // 0..31
    uint8_t role;           // 0..7
} GhdssStationId;

typedef struct {
    uint8_t version;
    uint8_t priority;
    uint8_t dest_type;
    uint8_t msg_type;
    uint8_t flags;
    uint8_t ttl;
    uint16_t msg_id;
    GhdssStationId originator;
    
    // Parsed ALERT fields (if msg_type == GHDSS_MSG_ALERT)
    uint8_t hazard_code;
    int32_t lat_scaled;     // Degrees * 100,000 (e.g. +4442677 -> 44.42677)
    int32_t lon_scaled;     // Degrees * 100,000 (e.g. +2610254 -> 26.10254)
    uint32_t utc_timestamp;
    uint8_t text_length;
    char detail_text[GHDSS_MAX_TEXT_LEN + 1];
    
    // Parsed ACK fields (if msg_type == GHDSS_MSG_ACK)
    GhdssStationId ref_originator;
    uint16_t ref_msg_id;
    uint8_t ack_result_code;
    
    // Checksum
    uint16_t received_crc;
    uint16_t calculated_crc;
    bool crc_valid;
} GhdssParsedPacket;

// ============================================================================
// Core Functions
// ============================================================================

// CRC-16-CCITT (Polynomial 0x1021, Init 0xFFFF)
uint16_t ghdss_crc16(const uint8_t *data, size_t length);

// Base40 Callsign Encoding / Decoding (48 bits / 6 bytes for 9 characters)
void ghdss_encode_base40(const char *callsign, uint8_t *out_bytes);
void ghdss_decode_base40(const uint8_t *in_bytes, char *out_callsign);

// Packet Builders
size_t ghdss_build_alert_packet(uint8_t *buffer, size_t max_len,
                                uint16_t msg_id,
                                const char *callsign, uint8_t ssid, uint8_t role,
                                uint8_t ttl,
                                int32_t lat_scaled, int32_t lon_scaled,
                                uint32_t utc_time,
                                uint8_t hazard_code,
                                const char *detail_text);

size_t ghdss_build_ack_packet(uint8_t *buffer, size_t max_len,
                              uint16_t msg_id,
                              const char *my_callsign, uint8_t my_ssid, uint8_t my_role,
                              const char *target_callsign, uint8_t target_ssid, uint8_t target_role,
                              uint16_t ref_msg_id,
                              uint8_t result_code);

// Packet Parser & Validator
bool ghdss_parse_packet(const uint8_t *buffer, size_t length, GhdssParsedPacket *out);

#ifdef __cplusplus
}
#endif

#endif // GHDSS_CODEC_H
