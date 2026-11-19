# GHDSS - Global Hamradio Distress and Safety System
## Client Implementation and Terminal Equipment Guidelines

* **Document Type:** Technical Implementation Guide & HMI Specification
* **Companion Document to:** [GHDSS Protocol Standard v1.0.0](file:///C:/Users/Eu/Desktop/GHDSS%20-%20Global%20Hamradio%20Distress%20and%20Safety/GHDSS_Protocol_Standard.md)
* **Status:** Specification
* **Edition:** 1.0.0

---

## 1. Purpose and Scope

This document defines the operational, safety, user interface (HMI), state engine, and gateway requirements for software developers building GHDSS client applications, mobile terminals, and network gateways.

While the core **GHDSS Protocol Standard** dictates Layer 3 on-air binary packet formats, this guide ensures ergonomic safety, uniform incident state management, and seamless inter-system communication.

---

## 2. Formal Incident State Engine

### 2.1 State Definitions
* **`IDLE`**: No active incident; terminal in monitoring mode.
* **`NEW`**: Outgoing alert generated; active retransmission cycle in progress.
* **`ACKNOWLEDGED`**: Positive Layer 3 ACK received from SAR/CONTROL; retransmission halted.
* **`ACTIVE`**: Incident ongoing; operational response received; tactical coordination active.
* **`CANCELLED`**: Originator cancelled alert (accidental trigger or self-cleared).
* **`CLOSED`**: Incident declared officially resolved and completed.

### 2.2 Formal State Transition Matrix

| Current State | Trigger / Event | Condition | Action Generated | Next State |
| :--- | :--- | :--- | :--- | :--- |
| **`IDLE`** | Manual Distress Triggered | 3-second hold completed | Emit `ALERT`, Start `ACK_WAIT_TIMER`, `Retry = 0` | **`NEW`** |
| **`NEW`** | `ACK_WAIT_TIMER` Expired | `Retry < DISTRESS_RETRY_COUNT` | Re-emit `ALERT`, `Retry++`, Restart Timer | **`NEW`** |
| **`NEW`** | `ACK_WAIT_TIMER` Expired | `Retry == DISTRESS_RETRY_COUNT` | Notify operator (`NO_ACK`), Enter Fallback | **`ACTIVE`** |
| **`NEW`** | Valid `ACK` Received | Ref Originator & Message ID match | Stop Timer, Sound success tone, Log ACK | **`ACKNOWLEDGED`** |
| **`ACKNOWLEDGED`** | `OPERATIONAL_RESPONSE` Recv | Response != `INCIDENT_CLOSED` | Notify operator (Status update), Update Log | **`ACTIVE`** |
| **`NEW` / `ACTIVE`** | Operator Cancel Initiated | False alarm or cleared by sender | Emit Distress Cancellation packet | **`CANCELLED`** |
| **`ACTIVE`** | `INCIDENT_CLOSED` Received | Emitted by `CONTROL` or `SAR` station | Sound incident-closed chime, Archive Log | **`CLOSED`** |
| **`CANCELLED`** | - | Immediate | Discontinue all relays, Transition to CLOSED | **`CLOSED`** |

---

## 3. Human-Machine Interface (HMI) and Safety Requirements

### 3.1 Accidental Trigger Mitigation (The 3-to-5 Second Rule)
To prevent catastrophic false emergency alarms, any physical or software interface for generating a `DISTRESS` alert **MUST** comply with the following:
* **Continuous Engagement:** Dedicated continuous press/hold for a minimum of 3.0 seconds (maximum 5.0 seconds).
* **Sensory Feedback:** A mandatory audible countdown chime and visual progress bar during engagement.
* **Abort on Release:** Release or cancellation prior to full duration **MUST** immediately abort transmission without on-air emission.

### 3.2 Acoustic and Visual Alerting
Receiving software implementations **MUST** provide distinct sensory alerting:
* **`DISTRESS` / `URGENCY`:** High-intensity, continuous or repeating acoustic alarm designed to awaken sleeping operators. The display screen **MUST** show a high-contrast red flashing banner displaying:
  * Originator Callsign-SSID
  * Incident Hazard Type
  * GPS Coordinates and Computed Distance/Bearing
  * Position Age and UTC Timestamp
* **`SAFETY`:** Intermittent single-pulse notification chime.
* **`ROUTINE`:** Standard subtle notification chime.

### 3.3 Distress Cancellation Ergonomics
Software interfaces **MUST** provide a clear, unambiguous **"CANCEL DISTRESS"** control allowing immediate transmission of a cancellation notice if triggered in error.

---

## 4. Gateway and External Interoperability

### 4.1 Common Alerting Protocol (CAP v1.2) Mapping
GATEWAY stations interfacing GHDSS with civil protection authorities (e.g. 112, DSU, FEMA) **SHALL** map GHDSS packets into OASIS CAP / ITU-T X.1303 XML documents:

| CAP Element | GHDSS Packet Field Mapping |
| :--- | :--- |
| `<identifier>` | `GHDSS-<Originator_ID>-<Message_ID>` |
| `<sender>` | `Callsign-SSID` |
| `<sent>` | ISO 8601 UTC Timestamp (derived from packet UTC Timestamp) |
| `<status>` | `"Actual"` (or `"Test"` if Message Type = 5) |
| `<msgType>` | `"Alert"` (or `"Ack"` / `"Cancel"`) |
| `<urgency>` | `"Immediate"` (for Priority = DISTRESS) |
| `<severity>` | `"Extreme"` (for Priority = DISTRESS) |
| `<certainty>` | `"Observed"` |
| `<area>` | `<circle>` formatted as `Latitude,Longitude Radius(km)` derived from Position Quality |

### 4.2 Winlink and Email Bridge
GATEWAY stations configured for Winlink forwarding **SHALL** automatically format incoming `DISTRESS` packets into standardized **ICS-213 Emergency Messages** and dispatch them to designated emergency email coordinators.

### 4.3 Authorized Payload Encryption Handling & Key Management

When handling packets with the `ENCRYPTED_PAYLOAD` flag set (Byte 3, Bit 0):

1. **Gateway Ingestion & Forwarding:**
   - **Authorized Gateway (with PSK):** If the Gateway has been provisioned with the civil authority incident PSK, it decrypts the payload, validates inner fields, and forwards the plaintext data into the secure authority dispatch network (e.g. OASIS CAP feed over TLS/HTTPS directly to 112/DSU dispatch).
   - **Relay Gateway (without PSK):** If the Gateway does not possess the key, it forwards the packet transparently across radio networks without modification, preserving the encrypted payload and header.

2. **HMI Key Injection & Terminal Safety:**
   - Client software **SHALL** provide a secure, simple UI for entering the Mission PSK (e.g., 32-character hex string or QR code scan from Incident Command).
   - If an encrypted packet is received without a valid PSK:
     - The terminal **SHALL** display an `"ENCRYPTED - KEY REQUIRED"` indicator.
     - The terminal **SHALL NOT** crash, hang, or discard the packet.
     - The terminal **SHALL** log the raw ciphertext and valid header metadata for audit purposes.

3. **High-Volume Data Handoff UX:**
   - If an operator attempts to compose a free-text message exceeding 128 characters, the terminal **SHALL** alert the operator: *"Alert payload limit reached (128 chars). For detailed reports, initiate a High-Speed ARQ Data Session."*
   - A single-click action **SHALL** allow transitioning the incident to a point-to-point VARA ARQ / Winlink session.

---

## 5. Recommended Emergency Calling Channels

### 5.1 High Frequency (HF) Emergency Centers of Activity (CoA)

| Band | Frequency | Preferred Contact Method | Primary Operational Usage |
| :--- | :--- | :--- | :--- |
| **80m** | **3760 kHz** | VARA HF / USB | Regional Night-time / NVIS EmComm (IARU Region 1 CoA) |
| **40m** | **7110 kHz** | VARA HF / USB | Regional Day-time EmComm (IARU Region 1 CoA) |
| **40m** | **7060 kHz** | FreeDATA | Digital Alternative Emergency CoA |
| **20m** | **14300 kHz** | VARA HF / USB | Global Intercontinental / Maritime Mobile Service Net |
| **15m** | **21360 kHz** | VARA HF / USB | Global Day-time CoA |

### 5.2 Very High Frequency (VHF) and Ultra High Frequency (UHF)

| Band | Frequency | Preferred Contact Method | Primary Operational Usage |
| :--- | :--- | :--- | :--- |
| **2m** | **144.800 MHz** | AX.25 / FX.25 (1200 bps) | Default IARU Region 1 Packet / APRS Shared Simplex |
| **2m** | **145.525 MHz** | VARA FM / FM Voice | Tactical VHF Simplex Emergency Channel |
| **70cm**| **433.500 MHz** | VARA FM / FM Voice | Tactical UHF Simplex Emergency Channel |
| **70cm**| **433.175 MHz** | LoRa (BW 125k, SF8, CR 4/5) | Dedicated Amateur Radio LoRa Distress & Tactical Simplex |
| **ISM** | **868.100 MHz** | LoRa (BW 125k, SF8, CR 4/5) | License-Exempt Inter-Agency SAR & Civil Protection Simplex |
