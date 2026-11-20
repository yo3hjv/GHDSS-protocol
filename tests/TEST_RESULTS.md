# GHDSS Test Execution Log & Results

**Protocol Standard:** GHDSS Layer 3 Specification Edition 1.0.0  
**Codec Implementation:** `ghdss_codec` v1.0.0 (CRC-16-CCITT `0x1021`, Base40 Callsign Compression)  
**Current Test Package:** [`TEST_BASIC_HANDSHAKE.rar`](file:///c:/Users/Eu/Desktop/GHDSS%20-%20Global%20Hamradio%20Distress%20and%20Safety/apps/TEST/TEST_BASIC_HANDSHAKE.rar)  
**Latest Verification:** November 20, 2026, 07:28:45 UTC+2  
**Main Working Directory:** `apps/TEST/`  

---

## 1. Testbed Hardware & RF Physical Layer

### Node Configurations

| Node Role | Station ID | MCU & Radio | Display & Bus | Firmware Directory |
| :--- | :--- | :--- | :--- | :--- |
| **Node A (Distress Sender)** | `YO3HJV-7` | ESP32-D0WDQ6 + SX1278 | 0.96" SSD1306 OLED (SDA=21, SCL=22) | `apps/TEST/ghdss_sender/` |
| **Node B (Responder / Gateway)** | `YO3HJV-1` | ESP32-D0WDQ6 + SX1278 | 0.96" SSD1306 OLED (SDA=21, SCL=22) | `apps/TEST/ghdss_receiver/` |

### LoRa PHY Settings (IARU Region 1 - 70cm Emergency Simplex)

| Parameter | Value | Standard Reference |
| :--- | :--- | :--- |
| **Frequency** | `433.175 MHz` | Primary UHF Emergency Simplex Channel |
| **Bandwidth (BW)** | `125.0 kHz` | Standard GHDSS Narrow Bandwidth |
| **Spreading Factor (SF)** | `SF8` | Fast emergency transaction baseline |
| **Coding Rate (CR)** | `4/5` (CR=5) | Error correction parity |
| **Sync Word** | `0x12` | GHDSS Layer 3 Sync Isolation |
| **TX Power** | `+10 dBm` | Bench-safe test level |
| **Preamble Length** | `8 symbols` | Standard preamble |

### Verified Hardware Pinout (LilyGO TTGO LoRa32 V1 / V2.0 No-TFCard)

```text
OLED Display (SSD1306):
  SDA = GPIO 21
  SCL = GPIO 22
  RST = NOT_CONNECTED (-1)
  I2C Address = 0x3C

SX1278 LoRa Transceiver:
  SCK  = GPIO 5
  MISO = GPIO 19
  MOSI = GPIO 27
  NSS  = GPIO 18
  DIO0 = GPIO 26 (Rx/Tx Done IRQ)
  RST  = GPIO 14
  DIO1 = GPIO 33
```

---

## 2. Test Cases & Verification History

### Test Case 1: Bilateral Over-The-Air Distress & ACK Handshake
* **Source Archive:** [`TEST_BASIC_HANDSHAKE.rar`](file:///c:/Users/Eu/Desktop/GHDSS%20-%20Global%20Hamradio%20Distress%20and%20Safety/apps/TEST/TEST_BASIC_HANDSHAKE.rar)
* **Date & Time:** November 19, 2026, 23:48:15 UTC+2
* **Trigger:** Manual trigger via Serial console (`t`)
* **Status:** **PASSED (100% Verified OTA)**

#### Transmission Log (Node A `YO3HJV-7` -> Node B `YO3HJV-1`)
* **Packet Type:** `GHDSS_MSG_ALERT` (`0x00`)
* **Packet Length:** 52 bytes
* **Air Time (ToA):** 188 ms
* **Raw Hex Stream:**
  ```hex
  47 48 1C 0C 60 03 EA DE AC 73 C0 00 00 38 1C C0 00 43 CA 35 00 27 D4 4E 00
  00 00 15 15 54 45 53 54 20 4D 45 44 49 43 41 4C 20 44 49 53 54 52 45 53 53
  1A C5
  ```
* **Decoded Header & Payload:**
  * **Magic:** `GH` (`0x47 0x48`)
  * **Priority / Flags:** `DISTRESS` (Priority 3), `ACK_REQUIRED` (`0x1C`)
  * **Originator:** Base40 packed callsign `YO3HJV-7`
  * **Message ID:** `1002`
  * **Hazard Code:** `0x04` (`MEDICAL`)
  * **Position:** `44.42677° N, 26.10254° E` (Bucharest, Romania)
  * **Detail Text:** `"TEST MEDICAL DISTRESS"` (21 characters)
  * **Integrity:** CRC-16-CCITT `0x1AC5` (Valid)

#### Reception & Response Log (Node B `YO3HJV-1`)
* **Signal Quality:** `RSSI = -27.00 dBm`, `SNR = +12.00 dB`
* **Response Action:** Automatic generation and transmission of binary `GHDSS_MSG_ACK`.
* **ACK Packet Length:** 26 bytes
* **Air Time (ToA):** 116 ms
* **Raw Hex Stream:**
  ```hex
  47 48 1C 10 20 07 D1 DE AC B7 14 00 00 09 DE AC 73 C0 00 00 38 03 EA 00 1F 03
  ```
* **Decoded ACK Fields:**
  * **Responder ID:** `YO3HJV-1` (SAR / Base Gateway)
  * **Reference Msg ID:** `1002`
  * **Result Code:** `0x00` (`ACCEPTED / VALID`)
  * **Integrity:** CRC-16-CCITT `0x1F03` (Valid)

#### Resolution on Node A
* **Confirmation Signal:** `RSSI = -25.00 dBm`, `SNR = +12.00 dB`
* **State Machine:** Transition from `STATE_WAITING_ACK` to `STATE_ACK_RECEIVED`.
* **Visual Indication:** OLED displays `*** ACK RECEIVED! *** Alert Cleared!`. Retransmission timers terminated.

---

### Test Case 2: ARQ Retransmission & Timeout Failover (No ACK Scenario)
* **Source Archive:** [`TEST_BASIC_HANDSHAKE.rar`](file:///c:/Users/Eu/Desktop/GHDSS%20-%20Global%20Hamradio%20Distress%20and%20Safety/apps/TEST/TEST_BASIC_HANDSHAKE.rar)
* **Date & Time:** November 20, 2026, 07:28:45 UTC+2
* **Scenario:** Node B offline / Responder unreachable.
* **Status:** **PASSED (100% Verified ARQ State Machine)**

#### Execution Timeline

```
T+00:00  [TX Attempt 1] MsgID: 1002, Retry: 0 | Len: 52 B | CRC: 0xC2CF | ToA: 188 ms
         Status: Listening for ACK (60s timer started)

T+01:00  [RETRY 1] ACK_WAIT_TIMER expired!
         [TX Attempt 2] MsgID: 1002, Retry: 1 | Len: 52 B | CRC: 0x871A | ToA: 188 ms
         Status: Timestamp incremented, CRC recalculated, listening for ACK (60s)

T+02:00  [RETRY 2] ACK_WAIT_TIMER expired!
         [TX Attempt 3] MsgID: 1002, Retry: 2 | Len: 52 B | CRC: 0x7913 | ToA: 188 ms
         Status: Timestamp incremented, CRC recalculated, listening for ACK (60s)

T+03:00  [FAIL] Maximum retries reached (MAX_RETRY_COUNT = 3)
         Event: "[FAIL] Maximum retries reached without ACK!"
         State Transition: STATE_NO_ACK
         Display: "NO ACK RECEIVED / Max retries reached!"
         Spectrum Protection: Radio transmission safely halted to prevent channel saturation.
```

---

## 3. Performance Summary Matrix

| Metric | Measured Value | Standard Limit / Target | Evaluation |
| :--- | :--- | :--- | :--- |
| **Distress Alert Air Time** | `188 ms` | `< 250 ms` (LoRa 125kHz SF8) | **Optimal** |
| **Responder Processing Delay** | `100 ms` | `50 - 500 ms` Turnaround Window | **Optimal** |
| **ACK Packet Air Time** | `116 ms` | `< 150 ms` | **Optimal** |
| **Total Round-Trip Time (RTT)** | `~404 ms` | `< 1000 ms` | **Ultra-Fast** |
| **ARQ Interval Accuracy** | `60.0 s ± 0.05 s` | `60 s nominal` | **Precise** |
| **Channel Flooding Prevention** | Guaranteed at Retry 3 | Stop after 3 unacknowledged attempts | **Compliant** |

---

## 4. Archive Index of Test Packages

| Archive Name | Description | Included Sketches | Validated Protocol Features |
| :--- | :--- | :--- | :--- |
| [`TEST_BASIC_HANDSHAKE.rar`](file:///c:/Users/Eu/Desktop/GHDSS%20-%20Global%20Hamradio%20Distress%20and%20Safety/apps/TEST/TEST_BASIC_HANDSHAKE.rar) | Baseline OTA Handshake & ARQ Retransmissions | `ghdss_sender/`<br>`ghdss_receiver/` | Binary Header Framing, Base40 Callsign Pack/Unpack, CRC-16-CCITT, LoRa Simplex SX1278 PHY, Automatic ACK, Timeout & Retries |
