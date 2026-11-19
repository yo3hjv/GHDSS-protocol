/*
 * ============================================================================
 * GHDSS - Global Hamradio Distress and Safety System
 * Node A: Distress Sender / Beacon (Class 1 Implementation)
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
// Station Configuration (Customizable)
// ============================================================================
const char MY_CALLSIGN[] = "YO3HJV";
const uint8_t MY_SSID    = 7;
const uint8_t MY_ROLE    = GHDSS_ROLE_OTHER;

// Test Coordinates (Bucuresti - KN34BK76)
const int32_t TEST_LAT_SCALED = 4442677; // +44.42677 deg * 100,000
const int32_t TEST_LON_SCALED = 2610254; // +26.10254 deg * 100,000
const char TEST_ALERT_TEXT[]  = "TEST MEDICAL DISTRESS";

// RF Configuration (IARU Region 1 - 70cm Emergency Simplex)
const float LORA_FREQ_MHZ = 433.175;
const float LORA_BW_KHZ   = 125.0;
const uint8_t LORA_SF     = 8;
const uint8_t LORA_CR     = 5;      // 4/5
const uint8_t LORA_SYNC   = 0x12;   // GHDSS LoRa Sync Word
const int8_t  LORA_POWER  = 10;     // dBm

// Operational Timers
const uint32_t ACK_TIMEOUT_MS  = 60000; // 60 seconds wait for ACK
const uint8_t  MAX_RETRY_COUNT = 3;     // Up to 3 retransmissions
const uint32_t HOLD_BUTTON_MS  = 3000;  // 3-second safety rule

// ============================================================================
// Global Objects & State Machine
// ============================================================================
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
SX1278 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RESET, LORA_DIO1);

bool oledAvailable = false;

enum SenderState {
    STATE_IDLE,
    STATE_HOLD_COUNTDOWN,
    STATE_TRANSMITTING,
    STATE_WAITING_ACK,
    STATE_ACK_RECEIVED,
    STATE_NO_ACK
};

SenderState currentState = STATE_IDLE;
uint16_t currentMsgId = 1001;
uint8_t currentRetry = 0;
uint32_t ackTimerStart = 0;
uint32_t buttonPressStart = 0;

uint8_t txPacketBuffer[GHDSS_MAX_PACKET_LEN];
size_t txPacketLen = 0;

uint8_t rxPacketBuffer[GHDSS_MAX_PACKET_LEN];
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
    display.print("GHDSS SENDER [");
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

// ============================================================================
// Action: Transmit Distress Alert
// ============================================================================
void sendDistressAlert() {
    currentState = STATE_TRANSMITTING;
    digitalWrite(LED_PIN, HIGH);

    char retryStr[24];
    snprintf(retryStr, sizeof(retryStr), "Attempt: %d/%d", currentRetry + 1, MAX_RETRY_COUNT);
    updateOled("TRANSMITTING ALERT", "Priority: DISTRESS", "Sending via LoRa...", retryStr);

    Serial.println();
    Serial.println("==================================================");
    Serial.print("[TX] Building GHDSS DISTRESS Alert (MsgID: ");
    Serial.print(currentMsgId);
    Serial.print(", Retry: ");
    Serial.print(currentRetry);
    Serial.println(")");

    // Build canonical binary packet
    txPacketLen = ghdss_build_alert_packet(
        txPacketBuffer, sizeof(txPacketBuffer),
        currentMsgId,
        MY_CALLSIGN, MY_SSID, MY_ROLE,
        3, // TTL = 3
        TEST_LAT_SCALED, TEST_LON_SCALED,
        (uint32_t)(millis() / 1000),
        GHDSS_HAZARD_MEDICAL,
        TEST_ALERT_TEXT
    );

    Serial.print("[TX] Packet length: ");
    Serial.print(txPacketLen);
    Serial.println(" bytes");
    Serial.print("[TX] Hex Stream: ");
    printHexDump(txPacketBuffer, txPacketLen);

    // Transmit via RadioLib
    uint32_t t_start = millis();
    int state = radio.transmit(txPacketBuffer, txPacketLen);
    uint32_t t_duration = millis() - t_start;

    digitalWrite(LED_PIN, LOW);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.print("[TX] Radio transmit SUCCESS (Time on air: ");
        Serial.print(t_duration);
        Serial.println(" ms)");
    } else {
        Serial.print("[TX] Radio transmit ERROR code: ");
        Serial.println(state);
    }

    // Switch to receive mode for ACK
    packetReceivedFlag = false;
    radio.startReceive();

    currentState = STATE_WAITING_ACK;
    ackTimerStart = millis();

    updateOled("WAITING FOR ACK...", "Alert sent!", "Listening 433.175MHz", "Timeout: 60s");
    Serial.println("[RX] Listening for GHDSS ACK...");
}

// ============================================================================
// Setup
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("##################################################");
    Serial.println("# GHDSS Node A (Distress Sender / Beacon)        #");
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
        updateOled("BOOTING SENDER", "Initializing LoRa...", "Freq: 433.175 MHz", "Please wait...");
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

    updateOled("SYSTEM READY", "Press BUTTON (3s)", "or type 't' in Serial", "to trigger Alert");
    Serial.println("[READY] System in IDLE. Press button (3s) or send 't' in Serial to trigger alert.");
}

// ============================================================================
// Main Loop
// ============================================================================
void loop() {
    // 1. Serial commands
    if (Serial.available()) {
        char c = Serial.read();
        if (c == 't' || c == 'T' || c == '\n' || c == ' ') {
            if (currentState == STATE_IDLE || currentState == STATE_ACK_RECEIVED || currentState == STATE_NO_ACK) {
                Serial.println("[CMD] Manual Distress Trigger received via Serial!");
                currentRetry = 0;
                currentMsgId++;
                sendDistressAlert();
            }
        }
    }

    // 2. Button Handling (3-second safety rule)
    bool buttonPressed = (digitalRead(BUTTON_PIN) == LOW);

    if (buttonPressed) {
        if (currentState == STATE_IDLE || currentState == STATE_ACK_RECEIVED || currentState == STATE_NO_ACK) {
            if (buttonPressStart == 0) {
                buttonPressStart = millis();
                currentState = STATE_HOLD_COUNTDOWN;
            } else {
                uint32_t heldTime = millis() - buttonPressStart;
                int remainingSec = (int)((HOLD_BUTTON_MS - heldTime) / 1000) + 1;
                if (remainingSec < 1) remainingSec = 1;

                char countStr[24];
                snprintf(countStr, sizeof(countStr), "Hold button: %d s", remainingSec);
                updateOled("SAFETY TRIGGER", "HOLD BUTTON TO ALERT", countStr, "Release to cancel");

                if (heldTime >= HOLD_BUTTON_MS) {
                    buttonPressStart = 0;
                    currentRetry = 0;
                    currentMsgId++;
                    sendDistressAlert();
                }
            }
        }
    } else {
        if (currentState == STATE_HOLD_COUNTDOWN) {
            buttonPressStart = 0;
            currentState = STATE_IDLE;
            updateOled("TRIGGER CANCELLED", "Button released early", "Press PRG to retry", "or 't' in Serial");
            Serial.println("[WARN] Button released before 3 seconds - alert cancelled.");
        }
    }

    // 3. Waiting for ACK processing
    if (currentState == STATE_WAITING_ACK) {
        if (packetReceivedFlag) {
            packetReceivedFlag = false;

            size_t rxLen = radio.getPacketLength();
            if (rxLen > 0 && rxLen <= sizeof(rxPacketBuffer)) {
                int readState = radio.readData(rxPacketBuffer, rxLen);
                if (readState == RADIOLIB_ERR_NONE) {
                    Serial.print("[RX] Received LoRa packet (");
                    Serial.print(rxLen);
                    Serial.println(" bytes)");
                    Serial.print("[RX] Hex Stream: ");
                    printHexDump(rxPacketBuffer, rxLen);

                    GhdssParsedPacket parsed;
                    if (ghdss_parse_packet(rxPacketBuffer, rxLen, &parsed)) {
                        Serial.println("[RX] GHDSS packet validated successfully!");

                        if (parsed.msg_type == GHDSS_MSG_ACK) {
                            Serial.print("[RX] ACK from: ");
                            Serial.print(parsed.originator.callsign);
                            Serial.print("-");
                            Serial.println(parsed.originator.ssid);
                            Serial.print("[RX] Ref Message ID: ");
                            Serial.println(parsed.ref_msg_id);

                            if (parsed.ref_msg_id == currentMsgId) {
                                currentState = STATE_ACK_RECEIVED;
                                digitalWrite(LED_PIN, HIGH);

                                float rssi = radio.getRSSI();
                                float snr = radio.getSNR();

                                Serial.println();
                                Serial.println("**************************************************");
                                Serial.println("*** POSITIVE ACK RECEIVED! DISTRESS RESOLVED! ***");
                                Serial.print("*** Responder: ");
                                Serial.print(parsed.originator.callsign);
                                Serial.print("-");
                                Serial.println(parsed.originator.ssid);
                                Serial.print("*** RSSI: ");
                                Serial.print(rssi);
                                Serial.print(" dBm, SNR: ");
                                Serial.print(snr);
                                Serial.println(" dB");
                                Serial.println("**************************************************");

                                char line1[32];
                                char line2[32];
                                snprintf(line1, sizeof(line1), "By: %s-%d", parsed.originator.callsign, parsed.originator.ssid);
                                snprintf(line2, sizeof(line2), "RSSI:%.0f SNR:%.1f", rssi, snr);
                                updateOled("*** ACK RECEIVED! ***", line1, line2, "Alert Cleared!");
                            }
                        }
                    }
                }
            }
            if (currentState == STATE_WAITING_ACK) {
                radio.startReceive();
            }
        }

        if (currentState == STATE_WAITING_ACK) {
            uint32_t elapsed = millis() - ackTimerStart;
            if (elapsed >= ACK_TIMEOUT_MS) {
                currentRetry++;
                if (currentRetry < MAX_RETRY_COUNT) {
                    Serial.println();
                    Serial.print("[RETRY] ACK_WAIT_TIMER expired! Retrying attempt ");
                    Serial.print(currentRetry + 1);
                    Serial.println("...");
                    sendDistressAlert();
                } else {
                    currentState = STATE_NO_ACK;
                    Serial.println();
                    Serial.println("[FAIL] Maximum retries reached without ACK!");
                    updateOled("NO ACK RECEIVED", "Max retries reached!", "Check radio/coverage", "Press PRG to retry");
                }
            } else {
                static uint32_t lastDisplayUpdate = 0;
                if (millis() - lastDisplayUpdate >= 1000) {
                    lastDisplayUpdate = millis();
                    int secLeft = (ACK_TIMEOUT_MS - elapsed) / 1000;
                    char timerStr[24];
                    snprintf(timerStr, sizeof(timerStr), "Wait ACK: %d s", secLeft);
                    char retryStr[24];
                    snprintf(retryStr, sizeof(retryStr), "Attempt: %d/%d", currentRetry + 1, MAX_RETRY_COUNT);
                    updateOled("LISTENING FOR ACK", "Freq: 433.175 MHz", retryStr, timerStr);
                }
            }
        }
    }

    delay(20);
}
