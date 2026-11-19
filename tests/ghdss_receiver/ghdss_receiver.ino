/*
 * ============================================================================
 * GHDSS - Global Hamradio Distress and Safety System
 * Node B: Responder / Net Control / Gateway (Class 2/3 Implementation)
 * Hardware: TTGO LoRa32 V1 (No TFCard) (ESP32 + SX1278 @ 433.175 MHz + OLED)
 * ============================================================================
 */

#include <SPI.h>
#include <Wire.h>
#include <RadioLib.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ghdss_codec.h"

// ============================================================================
// Hardware Pinout - TTGO LoRa32 V1 (No TFCard)
// ============================================================================
#define LORA_SCK      5
#define LORA_MISO    19
#define LORA_MOSI    27
#define LORA_NSS     18
#define LORA_DIO0    26
#define LORA_RESET   14
#define LORA_DIO1    33

#define OLED_SDA     21
#define OLED_SCL     22
#define OLED_RST     -1

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BUTTON_PIN    0   // Onboard PRG / BOOT button (Active LOW)
#define LED_PIN       2   // Onboard LED

// ============================================================================
// Station Configuration (Responder / SAR Unit)
// ============================================================================
const char MY_CALLSIGN[] = "YO3SAR";
const uint8_t MY_SSID    = 1;
const uint8_t MY_ROLE    = GHDSS_ROLE_SAR;

// RF Configuration (IARU Region 1 - 70cm Emergency Simplex)
const float LORA_FREQ_MHZ = 433.175;
const float LORA_BW_KHZ   = 125.0;
const uint8_t LORA_SF     = 8;
const uint8_t LORA_CR     = 5;      // 4/5
const uint8_t LORA_SYNC   = 0x12;   // GHDSS LoRa Sync Word
const int8_t  LORA_POWER  = 10;     // dBm

// ============================================================================
// Global Objects
// ============================================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RESET, LORA_DIO1);

bool oledAvailable = false;
uint16_t ackSequenceId = 2001;
uint8_t rxPacketBuffer[GHDSS_MAX_PACKET_LEN];
uint8_t txPacketBuffer[GHDSS_MAX_PACKET_LEN];

volatile bool packetReceivedFlag = false;

#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void onDio0Interrupt(void) {
    packetReceivedFlag = true;
}

// ============================================================================
// UI Helper Functions
// ============================================================================
void updateOled(const char *header, const char *line1, const char *line2, const char *line3) {
    if (!oledAvailable) return;

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    // Header banner
    display.setCursor(0, 0);
    display.print("GHDSS RESPONDER [");
    display.print(MY_CALLSIGN);
    display.print("-");
    display.print(MY_SSID);
    display.print("]");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);

    if (header) {
        display.setCursor(0, 14);
        display.print(header);
    }
    if (line1) {
        display.setCursor(0, 26);
        display.print(line1);
    }
    if (line2) {
        display.setCursor(0, 38);
        display.print(line2);
    }
    if (line3) {
        display.setCursor(0, 50);
        display.print(line3);
    }
    display.display();
}

void printHexDump(const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
        Serial.print(" ");
    }
    Serial.println();
}

const char* getHazardName(uint8_t code) {
    switch (code) {
        case GHDSS_HAZARD_GENERAL:     return "GENERAL";
        case GHDSS_HAZARD_FIRE:        return "FIRE";
        case GHDSS_HAZARD_FLOOD:       return "FLOOD";
        case GHDSS_HAZARD_EARTHQUAKE:  return "EARTHQUAKE";
        case GHDSS_HAZARD_MEDICAL:     return "MEDICAL";
        case GHDSS_HAZARD_LOST_PERSON: return "LOST PERSON";
        default:                       return "UNKNOWN";
    }
}

// ============================================================================
// Action: Send GHDSS ACK
// ============================================================================
void sendAck(const GhdssParsedPacket *alert) {
    digitalWrite(LED_PIN, HIGH);

    Serial.println();
    Serial.println("--------------------------------------------------");
    Serial.println("[TX] Formulating GHDSS ACKNOWLEDGEMENT (ACK)...");

    // Build canonical binary ACK packet
    size_t ackLen = ghdss_build_ack_packet(
        txPacketBuffer, sizeof(txPacketBuffer),
        ackSequenceId++,
        MY_CALLSIGN, MY_SSID, MY_ROLE,
        alert->originator.callsign, alert->originator.ssid, alert->originator.role,
        alert->msg_id,
        0 // Result Code 0 = Accepted / Valid
    );

    Serial.print("[TX] ACK length: ");
    Serial.print(ackLen);
    Serial.println(" bytes");
    Serial.print("[TX] Hex Stream: ");
    printHexDump(txPacketBuffer, ackLen);

    // Transmit ACK via LoRa
    uint32_t t_start = millis();
    int state = radio.transmit(txPacketBuffer, ackLen);
    uint32_t t_duration = millis() - t_start;

    // Reset flag because transmit asserted DIO0 (TX_DONE)
    packetReceivedFlag = false;

    digitalWrite(LED_PIN, LOW);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.print("[TX] ACK transmitted SUCCESS in ");
        Serial.print(t_duration);
        Serial.println(" ms");
    } else {
        Serial.print("[TX] ACK transmit ERROR code: ");
        Serial.println(state);
    }
    Serial.println("--------------------------------------------------");
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("##################################################");
    Serial.println("# GHDSS Node B (Responder / Gateway / SAR)       #");
    Serial.println("# Target: TTGO LoRa32 V1 (No TFCard)             #");
    Serial.println("# Layer 3 Protocol Edition 1.0.0                 #");
    Serial.println("##################################################");
    Serial.print("Station ID: ");
    Serial.print(MY_CALLSIGN);
    Serial.print("-");
    Serial.println(MY_SSID);

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // 1. Initialize OLED Display on Pins 21 (SDA), 22 (SCL)
    Serial.print("[INIT] Initializing I2C bus (SDA=21, SCL=22)... ");
    Wire.begin(OLED_SDA, OLED_SCL);
    if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C, false, false)) {
        oledAvailable = true;
        Serial.println("SUCCESS!");
        display.clearDisplay();
        updateOled("BOOTING RESPONDER", "Role: SAR GATEWAY", "Freq: 433.175 MHz", "Please wait...");
    } else {
        Serial.println("NOT DETECTED (Continuing headless)");
    }

    // 2. Initialize SPI explicitly for LoRa (SCK=5, MISO=19, MOSI=27, SS=18)
    Serial.print("[INIT] Initializing SPI bus (5, 19, 27, 18)... ");
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_NSS);
    Serial.println("OK");

    // 3. Initialize LoRa SX1278 (RadioLib v7 API)
    Serial.print("[INIT] Initializing SX1278 on 433.175 MHz... ");
    ConfigLoRa_t loraConfig;
    loraConfig.frequency = RADIOLIB_UNIT_MEGA(433.175);
    loraConfig.bandwidth = RADIOLIB_UNIT_KILO(125);
    loraConfig.spreadingFactor = LORA_SF;
    loraConfig.codingRate = LORA_CR;
    loraConfig.syncWord = LORA_SYNC;
    loraConfig.power = LORA_POWER;
    loraConfig.preambleLength = 8;

    int state = radio.begin(loraConfig);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("SUCCESS!");
        radio.setIrqAction(onDio0Interrupt);
    } else {
        Serial.print("FAILED, code: ");
        Serial.println(state);
        updateOled("RADIO INIT ERROR", "Check SX1278 wiring", "Check Serial log", NULL);
    }

    // Start listening
    state = radio.startReceive();
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[READY] Listening on 433.175 MHz for GHDSS alerts...");
        updateOled("LISTENING (STANDBY)", "Freq: 433.175 MHz", "Role: SAR Responder", "Waiting for Alerts");
    } else {
        Serial.print("[ERR] startReceive failed, code: ");
        Serial.println(state);
    }
}

// ============================================================================
// Main Loop
// ============================================================================
void loop() {
    if (packetReceivedFlag) {
        packetReceivedFlag = false;

        size_t rxLen = radio.getPacketLength();
        if (rxLen > 0 && rxLen <= sizeof(rxPacketBuffer)) {
            memset(rxPacketBuffer, 0, sizeof(rxPacketBuffer));
            int readState = radio.readData(rxPacketBuffer, rxLen);

            if (readState == RADIOLIB_ERR_NONE) {
                float rssi = radio.getRSSI();
                float snr  = radio.getSNR();

                Serial.println();
                Serial.println("==================================================");
                Serial.print("[RX] LoRa packet received: ");
                Serial.print(rxLen);
                Serial.println(" bytes");
                Serial.print("[RX] Signal: RSSI = ");
                Serial.print(rssi);
                Serial.print(" dBm, SNR = ");
                Serial.print(snr);
                Serial.println(" dB");
                Serial.print("[RX] Hex Stream: ");
                printHexDump(rxPacketBuffer, rxLen);

                // Parse packet with GHDSS Codec
                GhdssParsedPacket parsed;
                if (ghdss_parse_packet(rxPacketBuffer, rxLen, &parsed)) {
                    Serial.println("[RX] >>> VALID GHDSS PACKET DECODED <<<");
                    Serial.print("  * Version:      "); Serial.println(parsed.version);
                    Serial.print("  * Priority:     "); Serial.println(parsed.priority == GHDSS_PRIORITY_DISTRESS ? "DISTRESS (3)" : "OTHER");
                    Serial.print("  * Message Type: "); Serial.println(parsed.msg_type);
                    Serial.print("  * Originator:   "); Serial.print(parsed.originator.callsign); Serial.print("-"); Serial.println(parsed.originator.ssid);
                    Serial.print("  * Message ID:   "); Serial.println(parsed.msg_id);

                    if (parsed.msg_type == GHDSS_MSG_ALERT) {
                        float lat = parsed.lat_scaled / 100000.0f;
                        float lon = parsed.lon_scaled / 100000.0f;

                        Serial.println("  * --- ALERT PAYLOAD DETAILS ---");
                        Serial.print("  * Hazard:       "); Serial.println(getHazardName(parsed.hazard_code));
                        Serial.print("  * Position:     "); Serial.print(lat, 5); Serial.print(" N, "); Serial.print(lon, 5); Serial.println(" E");
                        Serial.print("  * Detail Text:  \""); Serial.print(parsed.detail_text); Serial.println("\"");

                        // Flash OLED with Emergency Information
                        char line1[32];
                        char line2[32];
                        char line3[32];
                        snprintf(line1, sizeof(line1), "FROM:%s-%d (%s)", parsed.originator.callsign, parsed.originator.ssid, getHazardName(parsed.hazard_code));
                        snprintf(line2, sizeof(line2), "%.4fN, %.4fE", lat, lon);
                        snprintf(line3, sizeof(line3), "Sending ACK... [%d]", parsed.msg_id);

                        updateOled("! DISTRESS ALERT !", line1, line2, line3);

                        // If ACK_REQUIRED is set, immediately respond with GHDSS ACK
                        if (parsed.flags & GHDSS_FLAG_ACK_REQUIRED) {
                            delay(100); // Small turnaround delay
                            sendAck(&parsed);

                            snprintf(line3, sizeof(line3), "ACK SENT! RSSI:%.0f", rssi);
                            updateOled("! DISTRESS ALERT !", line1, line2, line3);
                        }
                    } else if (parsed.msg_type == GHDSS_MSG_ACK) {
                        Serial.println("  * Type: ACKNOWLEDGEMENT");
                        Serial.print("  * Ref Callsign: "); Serial.println(parsed.ref_originator.callsign);
                        Serial.print("  * Ref Msg ID:   "); Serial.println(parsed.ref_msg_id);
                    }
                } else {
                    Serial.println("[RX] Error: Packet discarded (Invalid GHDSS header or CRC mismatch).");
                }
                Serial.println("==================================================");
            }
        }

        // Resume receiving
        packetReceivedFlag = false;
        radio.startReceive();
    }

    delay(20);
}
