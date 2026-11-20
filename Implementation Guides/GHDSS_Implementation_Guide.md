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

### 4.4 Automated Net Roll-Call & Roster Discovery Procedure

To establish and maintain an accurate operational inventory of active field stations without channel congestion, CONTROL and Field terminals **SHALL** implement the three-phase collision-free discovery procedure:

```
[ CONTROL Station ]                                           [ Field Nodes (N=1..50+) ]
        │                                                                  │
        │── Phase 1: Broadcast STATUS_REQUEST (Open Window 10s) ──────────>│
        │                                                                  │
        │<── Slotted Random Backoff (LBT) CHECK_IN (ACK_REQUIRED=0) ───────│ (Station A, B, C...)
        │    (Collsion occurs between Station D and E)                     │
        │                                                                  │
        │── Phase 2: Paged Burst NET_ROSTER_ECHO (Frames 1/M .. M/M) ─────>│ (All nodes listen)
        │    [ STRICT FIELD RADIO SILENCE ENFORCED DURING BURST ]          │
        │                                                                  │
        │    (Station A, B, C find their Callsign -> Mark CONFIRMED/IDLE)  │
        │    (Station D, E see sequence ended without their Callsigns)     │
        │                                                                  │
        │    [ Guard Delay: 500 ms ]                                       │
        │                                                                  │
        │── Phase 3: Recovery Window ──────────────────────────────────────│
        │<── Randomized Backoff (0.5 - 3.0s) CHECK_IN ─────────────────────│ (Only Station D and E)
        │                                                                  │
        ▼ (Roster 100% Complete)                                           ▼
```

#### Detailed Phase Specifications:

1. **Phase 1: Open Discovery Window**
   - **Trigger:** CONTROL station broadcasts `CONTROL_COMMAND` (Code 7: `STATUS_REQUEST` or Code 0: `CONTACT_REQUEST`) addressed to `ALL_STATIONS` (or filtered by `GRID` / `GROUP_ID` if $N > 50$).
   - **Field Node Behavior:** Upon reception, every active station computes a randomized slot delay ($T_{\text{slot}} \in [0.5\text{ s}, 10.0\text{ s}]$), evaluates the channel using LBT, and transmits its `CHECK_IN` packet (Message Type 6, Control Code 13).
   - **Crucial Rule:** Field stations **SHALL NOT** set `ACK_REQUIRED` during open discovery (`ACK_REQUIRED = 0`).

2. **Phase 2: Paged Group Confirmation (`NET_ROSTER_ECHO`)**
   - At the expiration of the collection window, CONTROL compiles all decoded calls into paginated frames of up to 16 stations each (7 bytes per station: 6 bytes Base40 + 1 byte SSID/Role).
   - CONTROL broadcasts the sequence with an inter-frame turnaround guard interval appropriate for the physical modem (e.g., 50 ms for solid-state VHF/UHF transceivers; 300–500 ms for HF transceiver PTT and relay switching):
     $$\text{Frame } 1\text{ of } M \longrightarrow \text{Frame } 2\text{ of } M \longrightarrow \dots \longrightarrow \text{Frame } M\text{ of } M$$
   - **Strict Field Radio Silence:** Field terminals **SHALL ENFORCE STRICT RADIO SILENCE** while $CurrentFrame < TotalFrames$. No terminal is permitted to transmit between frames.
   - **Confirmed Stations:** Terminals locating their callsign in any received frame **SHALL** transition immediately to `NET_REGISTERED` state and remain silent.

3. **Phase 3: Recovery Round for Unconfirmed Stations**
   - **Missing Station Detection:** A field station concludes it was dropped or collided **ONLY IF**:
     - Frame $M \text{ of } M$ was received and its callsign was absent across all frames, OR
     - An inactivity channel timeout of twice the nominal frame duration elapsed following the last received burst frame without reaching $M \text{ of } M$.
   - **Recovery Transmission:** Following frame $M \text{ of } M$ plus a mandatory **Guard Delay** (typically 500 ms on VHF/UHF; 1.5–2.0 s on HF), unconfirmed stations transmit their `CHECK_IN` using a randomized backoff interval with LBT. Because contention is reduced to only the missed stations, packet delivery probability approaches 100%.

4. **Network Scaling Beyond 50 Stations:**
   - When net size exceeds 50 stations, CONTROL stations **SHOULD** segment roll-call windows geographically using `GRID` addressing (Section 4.1) or organizational `GROUP_ID` addressing, preventing Aloha channel saturation.

#### 4.4.1 Relative Time Synchronization and Deterministic Slotting (Bearer-Agnostic Zero-Infrastructure Clocking)

A critical architectural principle of GHDSS Layer 3 is that it is **strictly bearer-agnostic**: the protocol operates identically over HF skywave channels (using soundcard modems like VARA HF, RTTY, FreeDATA, ARDOP) as it does over VHF/UHF tactical channels (AX.25 packet radio, VARA FM, LoRa).

Field stations operating inside disaster zones, deep gorges, or operating portable HF rigs in the field frequently lack GPS satellite lock or internet NTP connections. To achieve collision-free slotted roll-calls across **any physical radio modem** without requiring absolute real-time clocks:

1. **Relative Trigger Synchronization ($T_0$ Baseline):**
   - Stations **DO NOT** use absolute UTC wall-clock time for slot alignment.
   - The time origin ($T_0$) is established dynamically by the **physical completion of frame decoding** of the CONTROL station's broadcast message:
     - On soundcard/software modems (VARA HF, FreeDATA, ARDOP, RTTY): signaled by the modem's Data Carrier Detect (DCD) drop, KISS frame delimiter, or TCP stream `DISCONNECT`/frame end event.
     - On hardware packet controllers (AX.25 TNC): signaled by the valid FCS / CRC frame delivery event.
     - On integrated transceivers (LoRa): signaled by the RxDone hardware interrupt.
   - At this precise trigger event, every terminal latches its local monotonic millisecond clock:
     $$T_0 = \text{millis}()$$
   - Because electromagnetic waves propagate at $3 \times 10^8\text{ m/s}$, all stations across a tactical or regional radius (whether 10 km on UHF or 500 km via HF NVIS) establish $T_0$ within $\le 2\text{ ms}$ of each other. Local microcontroller quartz oscillators drift less than $0.2\text{ ms}$ over typical roll-call durations, providing laboratory-grade slotting without external time sources.

2. **Deterministic Slot Calculation (Mitigating Regional Callsign Clustering):**
   - **Prefix Collision Risk:** Stations in the same operational district share prefix letters and district digits (e.g., in Romania, all local Bucharest stations share `YO3...`; in Germany, regional nets share `DL...`). Deriving slots from the first letter or digit causes 100% of local stations to collide in the identical slot.
   - **Station Suffix & CRC Dispersion:** Terminals **SHALL** calculate their transmission slot using either of the following uniform dispersion methods:
     - **Method 1 (Dynamic Round-Salted CRC-16 - Recommended):**
       Using the station's 7-byte canonical identifier (6 bytes Base40 + 1 byte SSID/Role) and the active roll-call round number $R \in \{1, 2, 3\}$:
       $$\text{SlotIndex}(R) = \Big(\text{CRC16\_CCITT}(\text{StationID}) \oplus (R \times \text{0x5A5A})\Big) \pmod{N_{\text{slots}}}$$
       *(Incorporating round salt $R$ guarantees that any two stations that accidentally shared a slot in Round 1 will be re-dispersed into completely distinct slots in Round 2).*
     - **Method 2 (Suffix Character Indexing):**
       Derived from the final alphabetical character of the callsign suffix:
       $$\text{SlotIndex} = (\text{toupper}(\text{Callsign}[\text{last}]) - \text{'A'}) \pmod{N_{\text{slots}}}$$

3. **Bearer-Adaptive Transmission Formulation:**
   Each field node schedules its single `CHECK_IN` transmission at:
   $$T_{\text{tx}} = T_0 + T_{\text{turnaround}} + (\text{SlotIndex} \times T_{\text{slot\_duration}}) + T_{\text{jitter}}$$

   Where:
   - $T_{\text{turnaround}}$: Transceiver PTT / relay switching latency plus modem pre-carrier sync delay (typically 300–500 ms for HF transceivers; 50–100 ms for solid-state VHF/UHF).
   - $T_{\text{slot\_duration}}$: Total slot allocation composed strictly of actual Time-on-Air plus an **Inter-Slot Guard Interval**:
     $$T_{\text{slot\_duration}} = T_{\text{ToA}}(\text{Modem, 30 bytes}) + T_{\text{guard\_slot}}$$
   - $T_{\text{jitter}}$: Sub-slot pseudo-random micro-offset ($10\text{ to } 40\text{ ms}$) providing intra-slot phase separation for LBT evaluation.

4. **Inter-Slot Guard Intervals ($T_{\text{guard\_slot}}$) and Hardware Tolerances:**
   Without explicit inter-slot guard time, hardware variations between disparate amateur stations will cause packet tails from Slot $N$ to bleed into the preamble of Slot $N+1$, destroying demodulator synchronization at the CONTROL receiver. Terminals **SHALL** budget $T_{\text{guard\_slot}}$ to absorb the following real-world hardware factors:
   - **Mechanical T/R Relay and PTT Release Lag:** Mechanical relays in HF transceivers, antenna tuners (ATU), and linear amplifiers require 25–60 ms to release after RF drive stops.
   - **OS Audio/Soundcard Buffer Latency:** Software modems (VARA, ARDOP, RTTY soundcard AFSK) running under Windows/Linux incur 50–150 ms of DAC DMA buffer draining latency between software stop and actual audio line silence.
   - **Receiver AGC Recovery Time:** Following a strong local transmission (+40 dB over S9), the CONTROL receiver's Automatic Gain Control (AGC) decay circuitry requires 100–300 ms to regain full sensitivity before it can detect a weak distant signal in the subsequent slot.
   - **Microcontroller and Carrier Frequency Tolerances (CFO):** Accommodates slight thermal crystal drift and receiver carrier lock-in time.

5. **Probabilistic Contention Realities and Multi-Round Convergence:**
   - **Theoretical Foundation:** In open ad-hoc networks without centralized pre-assignment, no hashing scheme can mathematically eliminate collisions (by the Pigeonhole Principle and Birthday Paradox). Hashing is strictly a **probabilistic contention reduction mechanism**, dispersing $N$ stations across $S$ slots to reduce collision likelihood from $\ge 80\%$ (unslotted ALOHA) down to $\le 10–15\%$.
   - **Persistent Collision Elimination (Round Salt):** If two stations collide in Round 1, static hashing would force them to collide in every subsequent round. Incorporating the round salt $R$ into Method 1 guarantees mathematical re-hashing into independent slots during Round 2.
   - **Listen Before Talk (LBT) Intra-Slot Mitigation:** Sub-slot jitter ($T_{\text{jitter}}$) ensures that if two nodes share the same slot, one begins transmitting tens of milliseconds earlier; the second node detects the RF carrier and aborts its transmission, protecting the first packet from corruption.
   - **Physical Capture Effect:** In FM and LoRa modulations, a signal differential of 3–6 dB between co-slotted nodes allows the receiver to demodulate the stronger station cleanly (*Capture Effect*). The captured station is confirmed in `NET_ROSTER_ECHO`, leaving only the weaker station to retransmit unhindered in Round 2.
   - **Operational Convergence:**
     $$\text{Round 1 (Initial Roll-Call): } 85–90\% \text{ of active stations successfully registered}$$
     $$\text{Paged NET\_ROSTER\_ECHO: } \text{Confirms majority; only the unconfirmed } 10–15\% \text{ compete in Round 2}$$
     $$\text{Round 2 (Recovery Round): } \text{With contention density collapsed, net convergence exceeds } \mathbf{99.5\%}$$

#### Multi-Bearer Slot Sizing & Guard Interval Reference Table:

| Physical Layer / Modem | Bandwidth / Mode | Nominal ToA (30 B) | Inter-Slot Guard ($T_{\text{guard\_slot}}$) | Total Slot Duration ($T_{\text{slot}}$) | Hardware Driver Factors |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **HF RTTY (AFSK/FSK)** | 45.45 baud, 170 Hz shift | ~5.3 s | **1.2 s** | **6.5 s** | Audio buffer tail + mechanical relay settling |
| **HF RTTY (Fast)** | 75 baud, 170 Hz shift | ~3.2 s | **0.8 s** | **4.0 s** | Standard HF emergency RTTY |
| **VARA HF (Tactical 500)** | 500 Hz narrow channel | ~1.6 s | **600 ms** | **2.2 s** | Heavy QRM margin + HF AGC decay recovery |
| **VARA HF (Standard 2300)**| 2300 Hz standard SSB | ~0.8 s | **400 ms** | **1.2 s** | High-speed soundcard buffer flush |
| **FreeDATA / ARDOP** | 500 Hz OFDM robust | ~1.8 s | **700 ms** | **2.5 s** | Soundcard audio DMA latency + PTT delay |
| **AX.25 Packet (VHF/UHF)** | 1200 bps (Bell 202 AFSK) | ~260 ms | **190 ms** | **450 ms** | FM squelch tail + TNC TXDelay/tail |
| **LoRa (VHF/UHF)** | 125 kHz BW, SF8, CR 4/5 | ~188 ms | **212 ms** | **400 ms** | PLL synthesizer lock + SPI bus transfer |

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
