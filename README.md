# GHDSS — Global Hamradio Distress and Safety System
### An Open, Modern Emergency Alerting and Tactical Coordination Protocol for Amateur Radio

![Version](https://img.shields.io/badge/Version-1.0.0-blue.svg)
![Status](https://img.shields.io/badge/Status-Specification-brightgreen.svg)
![Layer](https://img.shields.io/badge/OSI-Layer%203%20(Network)-orange.svg)
![Amateur Radio](https://img.shields.io/badge/HamRadio-EmComm%20%2F%20SAR-red.svg)

---

## 1. Overview and Purpose

**GHDSS (Global Hamradio Distress and Safety System)** is an open, modern Layer 3 emergency alerting, network signaling, and tactical coordination protocol designed specifically for amateur radio emergency communications (EmComm), Search and Rescue (SAR) operations, and disaster response.

Inspired by the proven operational principles of the maritime **GMDSS** (*Global Maritime Distress and Safety System*) and **DSC** (*Digital Selective Calling*, ITU-R M.493), GHDSS transposes these critical life-safety concepts into the amateur radio ecosystem while adhering strictly to international amateur radio regulations (ITU RR 25.2A).

GHDSS acts as an **alerting, signaling, and channel coordination layer**: it rapidly delivers emergency notifications, geolocates victims and assets, enforces frequency discipline, and hands off high-volume operational traffic to dedicated voice or high-speed data channels.

---

## 2. Key Architectural Advantages

Why GHDSS represents a major evolution over legacy amateur emergency systems (such as plain APRS or uncoordinated voice calls):

### ⚡ 1. Extreme Spectral Efficiency & Strict Alerting Sizing
* **Minimal Over-The-Air Footprint:** Standard packets range from only **26 to 57 bytes** (with a theoretical maximum capped at **170 bytes**).
* **Immunity to Fading:** A 31-byte distress alert transmits in under **1.5 seconds** on HF, minimizing exposure to ionospheric QSB and atmospheric QRN.
* **Zero Fragmentation:** Fits comfortably within the standard 256-byte MTU limit of Packet Radio AX.25.
* **Signaling vs. High-Volume Handoff:** GHDSS is engineered strictly as an alerting and channel coordination layer; high-volume bulk data (casualty rosters, images, documents) is smoothly handed off via `DATA_SESSION_REQUEST` to dedicated Layer 2 ARQ channels (VARA ARQ / Winlink).

### 🌐 2. Pure Layer 3 Design (Modem-Agnostic)
* Decoupled from physical modulation: GHDSS runs identically across **VARA HF/FM**, **Packet Radio (AX.25 / FX.25)**, **LoRa (SX1262 / SX1276)**, **FreeDATA**, and **ARDOP**, utilizing their native Broadcast FEC (Forward Error Correction) engines.

### 🛡️ 3. Two-Tier Channel Sensing (Listen Before Talk — LBT)
* Prevents packet collisions on shared simplex channels.
* **Primary Sense:** Preamble correlation and Data Carrier Detect (DCD) detects digital GHDSS signals even **below the noise floor** (negative SNR on HF).
* **Secondary Sense:** RF energy / RSSI threshold prevents transmitting over existing analog voice (SSB/FM) conversations.

### 🔄 4. Controlled Mesh Flooding & Anti-Storm Routing
* Retransmission hops strictly governed by a decremented **TTL** (Time-To-Live).
* Automatic **Duplicate Message Cache** (24–72 hours) suppresses redundant packets.
* Randomized backoff timers automatically cancel duplicate relay transmissions if another station is heard repeating the packet.

### 🔐 5. Robust Security: Legal Anti-Spoofing & Authorized Civil Encryption
* **Open & Unencrypted by Default:** All standard operational payloads remain 100% human- and machine-readable in plain text, complying strictly with amateur radio licensing laws (ITU RR 25.2).
* **Anti-Spoofing & Anti-Replay Tokens:** Critical control directives (`HOLD_TRAFFIC`, `INCIDENT_NET_CONTROL`, `INCIDENT_CLOSED`) are authenticated with compact 32-bit one-time verification tokens (Ticket/OTP), preventing malicious takeovers or false alarms.
* **Authorized Payload Encryption (Civil Protection Mode):** When explicitly mandated by civil protection or public safety authorities during declared emergencies (for patient triage GDPR/HIPAA privacy or SAR tactical security), GHDSS supports zero-overhead stream encryption (AES-128-CTR / ChaCha20) via the `ENCRYPTED_PAYLOAD` flag. Layer 3 headers remain in plaintext, enabling transparent intermediate mesh relaying.

### 🗺️ 6. Sub-Meter GPS Precision & 8-Character Binary Maidenhead Geofencing
* GPS coordinates are encoded as signed 32-bit integers (`int32_t`, 1.1m resolution).
* Geographic addressing compactly encodes an entire **8-character Maidenhead locator** into just **4 bytes** (`uint32_t`), enabling high-resolution selective alerting on perimeters of $\approx 400 \times 500\text{ m}$.

### 🤝 7. Clear Functional Separation: ACK vs. Operational Response
* **`ACKNOWLEDGEMENT` (ACK):** Confirms technical radio reception by an authorized node, terminating the victim's retry loop.
* **`OPERATIONAL_RESPONSE`:** Separate operational status messages (`SAR_ACTIVE`, `TEAM_DISPATCHED`, `MEDICAL_SUPPORT_ENROUTE`), preventing false hopes while keeping everyone synchronized.

### 🚒 8. Dynamic Tactical SAR Capabilities & Resource Discovery
* Stations declare their operational specialization upon check-in via a 4-bit registry (e.g. *Ground Search, K9 Unit, Alpine Rescue, Medical Team, Air/Drone Support, Mobile Command Post*).
* Net Control stations can dynamically partition resources into tactical groups and assign dedicated working frequencies.

### 🚨 9. Native Civil Protection Interoperability (CAP v1.2 & Winlink)
* Gateways can translate GHDSS alerts directly into **OASIS CAP v1.2 XML** (Common Alerting Protocol) for seamless handoff to civil 112/911 dispatch centers and automated **Winlink ICS-213** emergency emails.

---

## 3. Project Documentation Structure

The standard is organized into two synchronized companion specifications:

1. 📄 **[GHDSS Protocol Standard v1.0.0 (Layer 3)](GHDSS_Protocol_Standard_v_x.x.x.md):**  
   The authoritative normative specification defining packet bitfields, state logic, registries (Hazard, Role, Capability), canonical algorithms in pseudo-code (Base40, Maidenhead, CRC-16), and official hex test vectors.
2. 📄 **[GHDSS Implementation Guide v1.0.0 (Terminals & HMI)](GHDSS_Implementation_Guide.md):**  
   The developer guide for building client software, detailing the formal state machine transition matrix, the 3-second safety button rule, acoustic alarms, Gateway CAP mappings, and recommended IARU emergency frequencies.

---

## 4. Community & Open Standards

GHDSS is developed as an open, royalty-free standard for the global amateur radio community. Contributions, software implementations, and field test feedback from national amateur radio societies (IARU member societies, ARES, EmComm, SAR teams) are warmly welcomed.

Author: Adrian Florescu YO3HJV
