# GHDSS - Global Hamradio Distress and Safety System
## Protocol Specification (Layer 3)

* **Document Type:** Normative Protocol Standard
* **Status:** Specification
* **Edition:** 1.0.0

---


---

## 1. Purpose


GHDSS is a radio amateur distress, urgency, safety and routine
alerting protocol inspired by the maritime GMDSS and DSC systems.

The protocol is intended for emergency communications,
disaster response, search and rescue support, public service
events and amateur radio emergency networks.


### 1.1 Architectural Scope (Layer 3 Protocol)


This Functional Specification Document defines GHDSS strictly as a
Layer 3 (Network, Operational Signaling, and Application) protocol.

Layer Responsibilities:

- Layer 1 (Physical / Modulation) and Layer 2 (Data Link / FEC):
  Delegated to underlying supported modems (e.g. VARA, ARDOP, FreeDATA,
  Packet AX.25). Channel coding, modulation, and Forward Error
  Correction (FEC) are handled natively by the modem.

- Layer 3 (GHDSS Network & Signaling):
  Handles station identification, message routing, priority queuing,
  geographic Maidenhead filtering, duplicate suppression, network-level
  operational acknowledgements (ACK), and incident coordination.

For alert broadcasts and signaling, GHDSS interacts with the modem
in Broadcast FEC (unproto) mode.


### 1.2 Normative Terminology (RFC 2119)


The key words "MUST", "MUST NOT", "REQUIRED", "SHALL", "SHALL NOT",
"SHOULD", "SHOULD NOT", "RECOMMENDED", "MAY", and "OPTIONAL" in this
document are to be interpreted as described in BCP 14, RFC 2119.


### 1.3 Conformance Classes


To accommodate diverse hardware constraints (from embedded low-power
trackers to autonomous repeaters and mobile command centers), GHDSS defines
four Conformance Classes:


#### 1.3.1 Class 1: Beacon and Alerting Device (Minimum Implementation)

- Intended for: Automated trackers, emergency distress beacons, unattended sensors.
- Mandatory features:
  - Generation and transmission of ALERT packets (Priority = DISTRESS).
  - Reception and parsing of ACKNOWLEDGEMENT packets to terminate retransmissions.
  - Transmission of basic CHECK_IN packets.
- Excluded: Packet relaying, network traffic control processing, session negotiation.


#### 1.3.2 Class 1R: Autonomous Tactical Relay Station (Digipeater)

- Intended for: Unattended mountaintop solar relays, tactical VHF/UHF digipeaters, automated gap-filler infrastructure nodes.
- Scope and Spectrum Constraints:
  - **VHF/UHF Operational Scope:** Class 1R autonomous relaying is strictly designated for VHF/UHF line-of-sight networks (e.g., LoRa 433/868 MHz, AX.25 VHF packet).
  - **HF Operational Restriction:** Autonomous, uncoordinated relaying (Class 1R) is **STRICTLY PROHIBITED** on HF skywave frequencies (below 30 MHz) to prevent catastrophic channel saturation, QRM, and self-interference on shared international emergency frequencies. Relaying on HF spectrum shall be conducted exclusively under Class 3 (Net Control / Gateway) coordination.
- Mandatory features:
  - Reception and validation of canonical GHDSS packets.
  - Enforcement of `RELAY_ALLOWED == 1` and `TTL > 0`.
  - Deduplication cache management (suppression of duplicate packets matching `Originator ID` + `Message ID`).
  - Packet header modification: TTL decrement (`TTL = TTL - 1`) and Hop Count increment (`Hop = Hop + 1`).
  - Strict Listen Before Talk (LBT) channel sensing (RSSI + Preamble Detect) and randomized backoff prior to retransmission.
  - Priority relaying: Forwarding packets with Priority `DISTRESS` (3) and `URGENCY` (2); routine traffic SHALL NOT be repeated.
  - Bidirectional relay support: Relaying both inbound `DISTRESS ALERT` and corresponding outbound `ACKNOWLEDGEMENT` packets.
  - Transparent forwarding: Intermediate relays SHALL transparently forward authorized encrypted payloads (`[ENCRYPTED - AUTHORIZED CIVIL PROTECTION]`) without requiring local decryption keys.
- Excluded: Local session negotiation, net control authority, incident state generation, local human UI/display requirements.


#### 1.3.3 Class 2: Field Responder Station

- Intended for: SAR ground/mobile units, field operators, paramedics.
- Mandatory features:
  - Full Class 1 capabilities.
  - Generation and reception of OPERATIONAL_RESPONSE and POSITION reports.
  - Reception of and compliance with FREQUENCY_CHANGE_REQUEST and GROUP_ASSIGNMENT.
  - Two-tier Listen Before Talk (LBT) sensing.
  - Optional: Ticket / OTP authentication verification.


#### 1.3.4 Class 3: Net Control and Gateway Station (Full Implementation)

- Intended for: Network Control Stations (CONTROL), SAR coordinators, Gateways.
- Mandatory features:
  - Full Class 1 and Class 2 capabilities.
  - Generation of INCIDENT_NET_CONTROL, HOLD_TRAFFIC, RESUME_TRAFFIC.
  - Full Ticket / OTP authentication support (generation and verification).
  - Controlled relaying, cross-network bridging, and deduplication cache management.
  - Cross-network / cross-mode gateway forwarding (including VHF/UHF LoRa to HF NVIS/Skywave bridges and CAP v1.2 / Winlink interfaces).


---

## 2. Station Identification


Each station shall be uniquely identified by:

CALLSIGN-SSID

Example:

YO3HJV-7


### 2.1 Callsign


The callsign length shall be between 1 and 9 characters.

Allowed characters:

A-Z
0-9
/
SPACE (padding)

Examples:

YO3HJV
YO3HJV/P
YR0HQ
4U1ITU


#### 2.1.1 Callsign Binary Encoding


To minimize packet overhead on HF/VHF, callsigns shall be
compactly encoded using Base40 (Radix-40) representation.

Base40 symbol set (40 values, index 0 to 39):
0: SPACE (padding)
1-10: '0'-'9'
11-36: 'A'-'Z'
37: '/'
38-39: RESERVED

Encoding structure:
Callsigns are packed using Base40 triplets into three 16-bit
words (40^3 = 64,000 < 65,536). Exactly 9 characters are packed
into 48 bits (6 bytes). Callsigns with fewer than 9 characters
are right-padded with SPACE (0).

Combined Station Identifier:
- Callsign: 48 bits (6 bytes, 9 characters)
- SSID: 5 bits (0-31)
- Padding/Flags: 3 bits
Total: 7 bytes (56 bits).


### 2.2 SSID


SSID range:

0-31

The SSID shall be represented using 5 bits.


### 2.3 Home Grid


Each station shall have a configured HOME_GRID.

HOME_GRID shall use the Maidenhead Locator System.

Minimum length:

6 characters

Recommended length:

8 characters

Examples:

KN34BK
KN34BK76

HOME_GRID shall be used for:

- geographic addressing
- group message filtering
- emergency communication organization
- routing extensions in future protocol versions


### 2.4 Station Role


Each GHDSS station shall have an assigned Station Role.

Defined roles:

**OTHER**
**SAR**
**EMCOMM**
**GATEWAY**
**CONTROL**
**RELAY**

Role definitions:

**OTHER**
General-purpose amateur radio station.

**SAR**
Search and Rescue station.

**EMCOMM**
Emergency Communications station.

**GATEWAY**
Station capable of forwarding messages between
different networks, modes or communication systems.

**CONTROL**
Network control station responsible for coordination
and traffic management.

**RELAY**
Autonomous infrastructure digipeater or tactical relay node
(typically operating as a Class 1R station on VHF/UHF).
Transparently forwards valid packets and does not originate distress alerts.

Default role:

**OTHER**


### 2.5 Tactical Capability


Each station participating in SAR or EMCOMM operations
may declare a Tactical Capability indicating its operational
specialization.

Field size:

4 bits

Supported values:

0 - 15

Tactical Capability assignments shall be maintained in
Appendix H (Tactical Capability Registry).

Default capability:

0 (UNSPECIFIED)


---

## 3. Message Priorities


* **0 ROUTINE**
* **1 SAFETY**
* **2 URGENCY**
* **3 DISTRESS**


---

## 4. Addressing Types


**ALL_STATIONS**
**INDIVIDUAL**
**GROUP**
**GRID**


### 4.1 Geographic Filtering


A station shall accept a GRID-addressed message if:

HOME_GRID starts with TARGET_GRID

Example:

HOME_GRID = KN34BK76
TARGET_GRID = KN34BK

Result:

ACCEPT

Example:

HOME_GRID = KN34BK76
TARGET_GRID = KN35

Result:

IGNORE


---

## 5. Hazard Codes


The protocol shall support 64 hazard codes.

Hazard Code field size:

6 bits

Hazard assignments shall be maintained in the
GHDSS Hazard Registry.


---

## 6. Position Information


Latitude and Longitude shall be encoded as signed 32-bit
integers (int32_t) in two's complement, Big-Endian
(Network Byte Order).

Binary encoding formula:

Latitude_Binary  = (int32_t) round(Latitude_Degrees * 100000)
Longitude_Binary = (int32_t) round(Longitude_Degrees * 100000)

Resolution:


### 0.00001 degree (approximately 1.1 meter at the equator)


Field size:

Latitude : 32 bits (4 bytes)
Longitude: 32 bits (4 bytes)
Total    : 64 bits (8 bytes)

Valid binary ranges:

Latitude : -9,000,000 to +9,000,000 (-90.00000 to +90.00000 deg)
Longitude: -18,000,000 to +18,000,000 (-180.00000 to +180.00000 deg)

Latitude and Longitude remain the primary position
reference for all DISTRESS and ALERT messages.


### 6.1 Position Quality


* **0 UNKNOWN**
* **1 MANUAL**
* **2 ESTIMATED**
* **3 GNSS**
* **4 DGNSS**
* **5 RTK**
* **6-15 RESERVED**


### 6.2 Position Age


* **0 CURRENT**
* **1 LESS_THAN_1_MINUTE**
* **2 BETWEEN_1_AND_5_MINUTES**
* **3 BETWEEN_5_AND_15_MINUTES**
* **4 BETWEEN_15_AND_60_MINUTES**
* **5 BETWEEN_1_AND_6_HOURS**
* **6 MORE_THAN_6_HOURS**
* **7 UNKNOWN**


### 6.3 Distress Position Requirements


The following fields are mandatory in all DISTRESS messages:

- Latitude
- Longitude
- UTC Timestamp

HOME_GRID is optional but recommended.


---

## 7. UTC Timestamp


UTC timestamp shall be included in all:

- DISTRESS
- URGENCY
- SAFETY

messages.


---

## 8. Acknowledgement and Relay Control


Each message shall contain:

- Originator ID
- Message ID
- TTL

The combination of Originator ID and Message ID
shall uniquely identify a message.

Stations shall maintain a temporary cache of
processed messages.

A message already processed shall not be
processed or relayed again.

Relay-capable stations shall decrement TTL
before retransmission.

Messages with TTL equal to zero shall not be
relayed.

Relay-capable stations shall employ a random
backoff timer before retransmission.

If a valid relay of the same message is received
during the backoff interval, the pending relay
shall be cancelled.

Relay operations on VHF/UHF tactical frequencies (e.g., 433 MHz LoRa)
may be conducted autonomously by Class 1R Digipeaters. On HF spectrum
(below 30 MHz), autonomous relaying is strictly prohibited; all HF
relaying and bridging shall be conducted under Class 3 coordination.


### 8.1 Distress Acknowledgements


The first valid acknowledgement received by
the originating station shall be considered
sufficient.

Additional acknowledgements are permitted but
are not required.

Acknowledgement-capable stations should use
a random backoff delay before transmitting
an acknowledgement.


### 8.2 Acknowledgement Eligibility


The following Station Roles are authorized
to acknowledge DISTRESS messages:

**SAR**
**EMCOMM**
**GATEWAY**
**CONTROL**

Stations with role OTHER shall not normally
transmit DISTRESS acknowledgements.


### 8.3 Channel Sensing and Listen Before Talk (LBT)


Prior to any transmission or retransmission (including
origination, acknowledgements and relays), stations shall
perform Listen Before Talk (LBT) channel evaluation.

A two-tier sensing architecture shall be employed:


---

## 1. Primary Detection: Packet Decode / Preamble Correlation

   The receiver evaluates the channel for active GHDSS
   preambles or valid data demodulation (Data Carrier Detect).
   This ensures detection of valid GHDSS traffic even when
   operating below the ambient noise floor (negative SNR).


---

## 2. Secondary Detection: Energy / RSSI Threshold

   The receiver measures RF energy across the channel bandwidth.
   If signal power exceeds the background noise floor by a
   configured threshold (e.g. +6 dB to +10 dB), the channel
   shall be considered occupied, preventing interference to
   voice (SSB/FM/AM) or other active transmissions.

Channel Busy Action:
If the channel is determined to be busy by either primary or
secondary detection:
- The pending transmission shall be deferred.
- Active backoff timers shall be suspended.
- Once the channel becomes clear, a Guard Interval (DIFS)
  shall elapse before resuming transmission or backoff countdown.


### 8.4 Distinction Between ACK and ARQ


GHDSS strictly differentiates between protocol-level operational
acknowledgement (ACK) and modem-level transport retransmission (ARQ):


---

## 1. ACK (Layer 3 Operational Acknowledgement):

   - Scope: GHDSS protocol level.
   - Nature: Connectionless (Broadcast / One-to-Many).
   - Operation: Discrete GHDSS packet (Message Type = ACKNOWLEDGEMENT)
     transmitted using modem Broadcast FEC mode.
   - Purpose: Confirms that an emergency alert or network command
     was successfully decoded and validated by an authorized station
     (SAR, CONTROL), terminating the originator's distress retry cycle.


---

## 2. ARQ (Layer 2 Modem-Level Automatic Repeat reQuest):

   - Scope: Modem / Data Link level (e.g. VARA ARQ, ARDOP ARQ).
   - Nature: Connection-oriented (Point-to-Point / Handshake).
   - Operation: Synchronous block-level retransmission handled
     internally by the modem software.
   - Purpose: Guaranteed error-free bulk data delivery (files, ICS
     forms, logs) during dedicated tactical sessions.


---

## 9. Message Packet Structure


Each GHDSS Layer 3 packet shall consist of:

PROTOCOL_ID
HEADER
ADDRESSING INFORMATION
PAYLOAD
AUTH_TOKEN (Optional)
CHECKSUM


### 9.1 Protocol Identifier


Used for network packet identification.

Value:

0x4748 (ASCII "GH")

Length:

16 bits (2 bytes)


### 9.2 Header


The Header shall contain:

- Protocol Version
- Message Type
- Priority
- Destination Type
- Flags
- Originator ID
- Message ID
- TTL


### 9.3 Message Types


The Message Type field (4 bits) identifies the functional
category of the packet, decoupled from the Priority field.

* **0 ALERT**
* **1 ACKNOWLEDGEMENT**
* **2 OPERATIONAL_RESPONSE**
* **3 POSITION**
* **4 TELEMETRY**
* **5 TEST**
* **6 CONTROL_COMMAND**
* **7-15 RESERVED**


#### 9.3.1 Priority vs Message Type Relationship


- Priority (2 bits, defined in Section 3) specifies channel
  preemption, queuing priority and emergency level:
  0 ROUTINE, 1 SAFETY, 2 URGENCY, 3 DISTRESS.
- Message Type (4 bits) specifies packet purpose and payload format.

An emergency alert is transmitted as:
Message Type = ALERT
Priority     = DISTRESS

An acknowledgement confirming an emergency alert is transmitted as:
Message Type = ACKNOWLEDGEMENT
Priority     = DISTRESS


### 9.4 Flags


**ACK_REQUIRED**
**RELAY_ALLOWED**
**AUTHENTICATED**
**ENCRYPTED_PAYLOAD**

Flag definitions:

**ACK_REQUIRED**
Indicates that receiving stations are requested to return an
ACKNOWLEDGEMENT packet upon technical receipt. For all DISTRESS
messages, ACK_REQUIRED shall be set.

**RELAY_ALLOWED**
Indicates that intermediate nodes may forward the packet if TTL > 0.

**AUTHENTICATED**
Indicates that an AUTH_TOKEN (Ticket/OTP) field is included immediately
preceding the Checksum as specified in Section 14 and Section 15.

**ENCRYPTED_PAYLOAD**
Indicates that the packet payload is encrypted end-to-end at the explicit
request of civil protection or emergency management authorities
(Section 14.6). The Layer 3 header remains in plaintext.

Optional future flags may be defined by later specification revisions.


### 9.5 Originator ID


Originator ID identifies the station that
originally created the message.

The Originator ID shall be encoded as a 56-bit (7-byte)
binary field comprising:
- Callsign: 48 bits (Base40 encoded, up to 9 characters)
- SSID: 5 bits (0-31)
- Padding/Role: 3 bits

Originator ID shall remain unchanged during all
relay operations.


### 9.6 Message ID


Each transmitted message shall contain a
Message ID.

The Message ID together with Originator ID
uniquely identifies a message.

The Message ID shall be used for:

- acknowledgement matching
- duplicate detection
- retransmission management
- relay suppression


### 9.7 Time To Live (TTL)


TTL limits the maximum number of relay hops.

Relay stations shall decrement TTL before
retransmission.

Messages with TTL equal to zero shall not be
relayed.

Recommended values:

DISTRESS: TTL = 3
URGENCY : TTL = 2
SAFETY  : TTL = 2
ROUTINE : TTL = 1


### 9.8 Payload


The Payload content depends on Message Type.

DISTRESS payload shall contain:

- Hazard Code
- Latitude
- Longitude
- Position Quality
- Position Age
- UTC Timestamp

Optional:

- Detail Text


### 9.9 Detail Text


Detail Text is optional.

Maximum length:

128 characters

Examples:

"2 PERSONS INJURED. POSSIBLE FRACTURE."

"VEHICLE IMMOBILIZED. NEED EVACUATION."

"LOST HIKER. COLD WEATHER. REQUEST SAR."


### 9.10 Integrity Check


All GHDSS packets shall contain a Checksum field.

The implementation shall use:

CRC-16-CCITT

Packets failing CRC validation shall be discarded.


---

## 10. Design Principles


- Automatic machine decoding
- Human-readable emergency information
- HF-friendly packet size
- Geographic filtering using Maidenhead locators
- Precise location using latitude and longitude
- Positive acknowledgement support
- Controlled relay architecture
- Duplicate suppression
- Relay loop prevention
- Extensible future message formats


---

## 11. Distress Workflow



### 11.1 General


The Distress Workflow defines the operational behavior
of GHDSS stations during the lifecycle of a DISTRESS
message.

Note: Throughout this workflow, a DISTRESS message refers
to a GHDSS ALERT packet (Message Type = 0) with the Priority
field set to DISTRESS (3).

The primary objectives are:

- reliable delivery
- positive acknowledgement
- controlled retransmission
- duplicate suppression
- prevention of relay loops
- efficient HF spectrum utilization


### 11.2 Station Configuration Parameters


The following parameters shall be configurable:

**ACK_WAIT_TIMER**

Time allowed for reception of a valid acknowledgement.

Default value:

60 seconds

Allowed range:

10 to 600 seconds

**DISTRESS_RETRY_COUNT**

Maximum number of retransmissions of a DISTRESS message.

Default value:

5

Allowed range:

1 to 255

**DISTRESS_RETRY_INTERVAL**

Time between retransmissions when no valid
acknowledgement has been received.

Default value:

**ACK_WAIT_TIMER**

**MESSAGE_RETRY_COUNT**

Maximum number of retransmissions for non-distress
messages requiring acknowledgement.

Default value:

3

**MESSAGE_RETRY_INTERVAL**

Default value:

60 seconds


### 11.3 Distress Origination


A DISTRESS message shall be generated by the
originating station when immediate assistance
is required.

A DISTRESS message shall contain:

- Originator ID
- Message ID
- Hazard Code
- Latitude
- Longitude
- Position Quality
- Position Age
- UTC Timestamp

Optional:

- HOME_GRID
- Detail Text

For all DISTRESS messages:

ACK_REQUIRED shall be set.

RELAY_ALLOWED should be set.


### 11.4 Distress Reception


A receiving station shall verify:

- packet integrity
- CRC validity
- message format
- duplicate status

Messages failing validation shall be discarded.

Messages already present in the Duplicate Message
Cache shall be discarded.


### 11.5 Distress Acknowledgement


The first valid acknowledgement received by the
originating station shall be considered sufficient.

Additional acknowledgements are permitted but
are not required.

The originating station shall transition the
message state to ACKNOWLEDGED upon receipt of
the first valid acknowledgement.

A DISTRESS acknowledgement confirms successful
reception of the message.

A DISTRESS acknowledgement does not imply that
assistance has been accepted or that resources
have been dispatched.


### 11.6 Acknowledgement Eligibility


The following Station Roles are authorized to
transmit DISTRESS acknowledgements:

- SAR
- EMCOMM
- GATEWAY
- CONTROL

Stations with role OTHER shall normally refrain
from transmitting DISTRESS acknowledgements.


### 11.7 Acknowledgement Backoff


Acknowledging stations shall employ a random
backoff timer before transmitting an
acknowledgement.

If a valid acknowledgement for the same
Originator ID and Message ID is received during
the backoff interval, the pending acknowledgement
shall be cancelled.


### 11.8 Distress Retransmission


If no valid acknowledgement is received within
ACK_WAIT_TIMER, the originating station shall
retransmit the DISTRESS message.

Retransmissions shall continue until:

- a valid acknowledgement is received, or
- DISTRESS_RETRY_COUNT is reached.

Each retransmission shall preserve:

- Originator ID
- Message ID


### 11.9 Relay Procedure


Relay-capable stations may retransmit a
DISTRESS message when:

- RELAY_ALLOWED is set
- TTL is greater than zero
- the message has not been processed previously

Prior to retransmission:

- TTL shall be decremented
- the original Originator ID shall be preserved
- the original Message ID shall be preserved

A relayed message remains a DISTRESS message.

Relay operations shall not modify the
Message Type.


### 11.10 Relay Suppression and Channel Sensing


Relay-capable stations shall perform Listen Before Talk (LBT)
channel evaluation (Section 8.3) prior to retransmission.

Relay-capable stations shall employ a random
backoff timer before retransmission.

If a valid relay of the same message is received
during the backoff interval, the pending relay
shall be cancelled.

If the channel is detected as busy during the backoff
interval (via packet preamble correlation or RSSI), the
countdown shall be paused until the channel is clear.

This mechanism reduces unnecessary retransmissions
and network congestion.


### 11.11 Duplicate Message Cache


Stations shall maintain a cache of processed
messages.

The cache key shall consist of:

- Originator ID
- Message ID

Entries shall be retained for a minimum of:

24 hours

Recommended retention time:

72 hours

Messages matching an existing cache entry
shall not be acknowledged or relayed.


### 11.12 Time To Live (TTL)


TTL limits the maximum number of relay hops.

Relay stations shall decrement TTL before
retransmission.

Messages with TTL equal to zero shall not be
relayed.

Recommended values:

DISTRESS : TTL = 3
URGENCY  : TTL = 2
SAFETY   : TTL = 2
ROUTINE  : TTL = 1


### 11.13 Message Delivery Modes


GHDSS shall support two delivery modes:

**CONFIRMED DELIVERY**

The sender requests acknowledgement.

**BROADCAST DELIVERY**

The sender does not request acknowledgement.

The selected mode shall be determined by the
ACK_REQUIRED flag.

ACK_REQUIRED = 1

Confirmed Delivery

ACK_REQUIRED = 0

Broadcast Delivery


### 11.14 Retransmission of Non-Distress Messages


Messages of type URGENCY, SAFETY and ROUTINE
may request acknowledgement.

If ACK_REQUIRED is set and no acknowledgement
is received within ACK_WAIT_TIMER, the message
may be retransmitted.

Retransmissions shall continue until:

- acknowledgement is received, or
- MESSAGE_RETRY_COUNT is reached.


### 11.15 Distress State Model


Each DISTRESS message shall transition through
one or more of the following states:

**NEW**
**ACKNOWLEDGED**
**ACTIVE**
**CANCELLED**
**CLOSED**

**NEW**

The message has been generated and transmitted.

**ACKNOWLEDGED**

At least one valid acknowledgement has been
received.

**ACTIVE**

The incident remains ongoing and assistance
may still be required.

**CANCELLED**

The originating station has transmitted a
Distress Cancellation message.

**CLOSED**

The incident has been resolved and the
Distress event is considered complete.


### 11.16 Distress Cancellation


A station originating a DISTRESS message
shall be able to transmit a Distress
Cancellation message.

The cancellation message shall reference:

- Originator ID
- Message ID

of the original DISTRESS message.

Upon successful reception of a valid
cancellation message, stations shall stop
acknowledging and relaying the referenced
DISTRESS message.


### 11.17 Distress Closure


A DISTRESS event shall be considered closed
when:

- a valid Distress Cancellation message
  has been received, or

- a designated CONTROL station declares
  the incident closed.


### 11.18 Operational Response Messages


A station may transmit an Operational Response
Message related to an existing DISTRESS event.

The Operational Response Message shall reference:

- Originator ID
- Message ID

of the associated DISTRESS message.

Examples:

- ASSISTANCE_ACCEPTED
- TEAM_DEPLOYED
- SAR_ACTIVE
- MEDICAL_SUPPORT_ENROUTE
- INCIDENT_TRANSFERRED

Operational Response Messages are distinct from
Acknowledgement messages.

An acknowledgement confirms message reception.

An Operational Response Message confirms
operational action.


### 11.19 Event Logging


Stations shall maintain a human-readable event log.

The log shall contain at minimum:

- UTC Timestamp
- Event Type
- Originator ID
- Message ID
- Message Type
- Result

Recommended additional fields:

- Position
- Hazard Code
- Remote Station
- Signal Information


### 11.20 Log Export


Stations should support export of event logs
using CSV format.

Recommended CSV fields:

UTC_TIMESTAMP,
EVENT_TYPE,
MESSAGE_TYPE,
MESSAGE_ID,
ORIGINATOR_ID,
DESTINATION,
* **RESULT**


### 11.21 Network Objectives


The Distress Workflow shall ensure:

- at least one successful acknowledgement
- controlled relay propagation
- duplicate suppression
- relay loop prevention
- efficient HF spectrum utilization
- interoperability between all GHDSS stations
- traceability through standardized logging


---

## 12. Message Definitions



### 12.1 General


GHDSS messages are categorized by Message Type.

Each Message Type defines:

- message purpose
- required fields
- optional fields
- operational behavior

All message types shall contain:

- Originator ID
- Message ID
- Timestamp

unless otherwise specified.


### 12.2 ALERT Message



#### 12.2.1 Purpose


The ALERT message is used to transmit distress, urgency,
or safety incident notifications. The incident severity
is determined by the Priority field in the packet header:

- Priority = DISTRESS (Immediate assistance required)
- Priority = URGENCY  (Urgent situation)
- Priority = SAFETY   (Safety broadcast)

When Priority is set to DISTRESS, the message represents
a DISTRESS ALERT.


#### 12.2.2 Mandatory Fields


An ALERT message shall contain:

- Hazard Code
- Latitude (32-bit signed binary integer)
- Longitude (32-bit signed binary integer)
- Position Quality
- Position Age
- UTC Timestamp


#### 12.2.3 Optional Fields


An ALERT message may contain:

- HOME_GRID
- Detail Text


#### 12.2.4 Delivery Requirements


DISTRESS-level ALERT messages shall:

- set ACK_REQUIRED
- support controlled relay
- support retransmission
- support duplicate suppression


### 12.3 ACKNOWLEDGEMENT Message



#### 12.3.1 Purpose


The ACKNOWLEDGEMENT message confirms successful
reception of another message.


#### 12.3.2 Message Correlation


An ACKNOWLEDGEMENT message shall reference:

- Originator ID
- Message ID

of the acknowledged message.


#### 12.3.3 Operational Meaning


An ACKNOWLEDGEMENT confirms:

- successful reception
- successful decoding
- successful validation

An ACKNOWLEDGEMENT does not confirm:

- operational action
- dispatch of resources
- acceptance of responsibility


#### 12.3.4 Multiple Acknowledgements


More than one ACKNOWLEDGEMENT may reference
the same message.

The first valid acknowledgement received by
the originating station shall be considered
sufficient.


### 12.4 OPERATIONAL RESPONSE Message



#### 12.4.1 Purpose


An OPERATIONAL RESPONSE message is used to
indicate actions taken in response to a
previously received DISTRESS message.

Operational Response Messages are independent
from Acknowledgement Messages.

Acknowledgement indicates successful reception.

Operational Response indicates operational
action.


#### 12.4.2 Message Correlation


Each Operational Response Message shall reference:

- Originator ID
- Message ID

of the associated DISTRESS message.


#### 12.4.3 Response Code


The message shall contain a Response Code.

Response Code assignments shall be maintained
in the GHDSS Response Registry.


#### 12.4.4 Optional Fields


An Operational Response Message may contain:

- Detail Text

Maximum Detail Text length:

128 characters


#### 12.4.5 Multiple Responses


More than one Operational Response Message may
reference the same DISTRESS message.

Stations shall not treat Operational Response
Messages as duplicate acknowledgements.


#### 12.4.6 Logging


Operational Response Messages shall be logged
together with the associated DISTRESS event.

The correlation shall be performed using:

- Originator ID
- Message ID


### 12.5 POSITION Message



#### 12.5.1 Purpose


The POSITION message is used to report the
current position of a station.


#### 12.5.2 Mandatory Fields


A POSITION message shall contain:

- Latitude
- Longitude
- Position Quality
- Position Age
- UTC Timestamp


#### 12.5.3 Optional Fields


A POSITION message may contain:

- HOME_GRID
- Altitude
- Course
- Speed
- Detail Text


### 12.6 TELEMETRY Message



#### 12.6.1 Purpose


The TELEMETRY message is used to transport
machine-readable status information.


#### 12.6.2 Typical Uses


Examples include:

- battery status
- power system status
- sensor readings
- weather measurements
- repeater status
- gateway status


#### 12.6.3 Data Format


Telemetry payload format shall be defined
by future specification revisions.


### 12.7 TEST Message



#### 12.7.1 Purpose


The TEST message is used to verify:

- connectivity
- protocol functionality
- station readiness
- operator readiness


#### 12.7.2 Operational Requirements


TEST messages shall not initiate emergency
procedures.

TEST messages shall not create active incidents.


#### 12.7.3 Acknowledgement


TEST messages may request acknowledgement.


### 12.8 CONTROL_COMMAND Message



#### 12.8.1 Purpose


The CONTROL_COMMAND message carries Automation and
Control Services requests, coordination directives, and
traffic management commands as specified in Section 13.


#### 12.8.2 Payload Structure


The payload contains the specific Control Message Code
(defined in Appendix G) and associated operational parameters.


### 12.9 Future Message Types


Additional message types may be defined by
future revisions of the specification.

Implementations shall ignore unsupported
message types while preserving normal
protocol operation.


### 12.10 Registries


The following registries shall be maintained
separately from this specification:

- Hazard Registry
- Response Registry
- Station Role Registry
- Contact Method Registry
- Automation and Control Message Registry
- Tactical Capability Registry

Registry contents may evolve without requiring
modification of the core protocol specification.



---

## 13. Automation and Control Services



### 13.1 Purpose


Automation and Control Services provide a standardized
mechanism for communication coordination, traffic
management and operational control between GHDSS stations.

These services allow stations to:

- establish contact
- select communication methods
- select operating frequencies
- request operational information
- initiate tracking functions
- coordinate incident response activities
- manage channel occupancy during incidents

Automation and Control Services extend the alerting
capabilities of GHDSS and support operational
coordination after message delivery.


### 13.2 Communication Profile


Each station shall maintain a Communication Profile.

The Communication Profile shall contain:

- Contact Channels
- Station Role
- Optional Capability Information


### 13.3 Contact Channels


A station may define up to four preferred
Contact Channels.

Each Contact Channel consists of:

- Contact Method
- Operating Frequency

Contact Methods are defined in the
Contact Method Registry.

Operating Frequency shall be specified
explicitly by the station operator.


### 13.4 Contact Channel Priority


Contact Channels shall be stored in priority order.

Channel 1 shall represent the station's preferred
communication channel.

Channel 4 shall represent the lowest-priority
configured communication channel.

Receiving stations should attempt contact using
the highest-priority channel available.


### 13.5 Automation and Control Messages


Automation and Control Messages provide a
standardized method for requesting operational
actions from remote stations.

Automation and Control Message definitions shall
be maintained in the Automation and Control
Message Registry.


### 13.6 Contact Request


A Contact Request is used to request direct
communication with a station.

The request may specify:

- preferred Contact Method
- preferred Operating Frequency


### 13.7 Frequency Change Request


A Frequency Change Request is used to request
migration of communications to a different
frequency.

The request shall specify:

- Operating Frequency
- Contact Method


### 13.8 Mode Change Request


A Mode Change Request is used to request migration
to a different Contact Method.


### 13.9 Data Session Request


A Data Session Request is used to negotiate and establish
a dedicated point-to-point data communication session.

The request shall identify:

- Contact Method
- Operating Frequency

Upon accepting a Data Session Request, stations transition from
GHDSS Layer 3 broadcast signaling to the designated tactical channel,
initiating a native Layer 2 Modem ARQ connection (e.g. VARA ARQ)
for high-volume payload exchange.


### 13.10 Position Request


A Position Request is used to request the
current position of a station.


### 13.11 Position Tracking Request


A Position Tracking Request is used to request
periodic transmission of position reports.

The request may contain:

- reporting interval
- reporting duration


### 13.12 Tracking Termination Request


A Tracking Termination Request is used to
terminate a previously established position
tracking session.


### 13.13 Status Request


A Status Request is used to request operational
information from a station.

Examples include:

- station status
- battery status
- communication status
- role information
- capability information


### 13.14 Traffic Control Messages


Traffic Control Messages are used to manage
channel occupancy during emergencies,
incidents, exercises and coordinated operations.

Traffic Control Messages may be addressed to:

- an individual station
- a group
- a geographic area
- all stations


### 13.15 Hold Traffic Request


A HOLD_TRAFFIC Request is used to reduce or
suspend lower-priority traffic.

The request may contain:

- Target
- Minimum Allowed Priority
- Duration
- Reason

Examples of Reason values:

- DISTRESS_TRAFFIC
- SAR_OPERATION
- NETWORK_CONGESTION
- EXERCISE


### 13.16 Hold Traffic Behaviour


Stations receiving a valid HOLD_TRAFFIC Request
shall suppress transmissions below the specified
Minimum Allowed Priority.

Example:

MINIMUM_PRIORITY = SAFETY

Permitted:

- DISTRESS
- URGENCY
- SAFETY

Suppressed:

- ROUTINE


### 13.17 Resume Traffic Request


A RESUME_TRAFFIC Request is used to terminate
a previously established HOLD_TRAFFIC condition.

The request may contain:

- Target
- Reason

Upon reception of a valid RESUME_TRAFFIC Request,
stations shall resume normal traffic procedures.


### 13.18 Standby Request


A STAND_BY Request is used to request that a
station remain available for subsequent
communications.


### 13.19 Operator Authority


Automation and Control Services shall assist
operators but shall not replace operator control.

The operator shall retain authority over:

- frequency selection
- mode selection
- transmission decisions
- acceptance of requests
- execution of requested actions


### 13.20 Future Extensions


Additional Automation and Control Message types
may be defined through the Automation and Control
Message Registry without requiring modification
of the core GHDSS specification.



---

## 14. Authentication and Anti-Spoofing Services



### 14.1 Purpose


Authentication and Anti-Spoofing Services provide origin verification
and message integrity for GHDSS operations without violating amateur
radio regulations prohibiting encrypted transmissions.

All GHDSS payload content remains 100% plain text (unencrypted) and
human/machine-readable by any listening station. Verification is
provided by appending a compact, one-way Ticket / OTP verification token.


### 14.2 Architecture and Ticket / OTP Generation


Authentication relies on a lightweight keyed hash mechanism (e.g.
HMAC / TOTP time-window truncation):

- Token Size: 32 bits (4 bytes)
- Input parameters:
  - Incident Pre-Shared Key (PSK) or Group Secret
  - Originator ID (Callsign-SSID)
  - Message ID
  - UTC Timestamp (quantized to time window, e.g. 60 or 120 seconds)

Calculation model:
  Ticket = Truncate(HMAC(Incident_Secret, Originator_ID || Message_ID || Time_Window))


### 14.3 Operational Policies



#### 14.3.1 Distress Alerts (Role: OTHER)

Authentication shall be OPTIONAL for general distress alerts. Stations in
distress without configured credentials or incident keys shall always be
processed and never rejected.


#### 14.3.2 Network Control and Coordination (Roles: CONTROL, SAR)

Authentication should be MANDATORY for critical coordination directives:
- INCIDENT_NET_CONTROL
- HOLD_TRAFFIC / RESUME_TRAFFIC
- INCIDENT_CLOSED
- ASSISTANCE_ACCEPTED

Receiving stations operating in an authenticated incident net shall
verify the Ticket. Control commands failing authentication shall be
flagged as untrusted and rejected.


### 14.4 Replay Attack Mitigation


Replay attacks are prevented through the combination of:
- Time-window validity of the Ticket/OTP
- Message ID correlation
- Duplicate Message Cache (replayed packets matching processed IDs
  are automatically discarded)


#### 14.4.1 Clock Drift and Window Tolerance


Stations operating in disaster areas without direct GNSS or NTP
time synchronization may experience clock drift.

To accommodate minor clock discrepancies, receiving stations
shall validate the Ticket against the current time window as well
as adjacent windows within an operator-configurable tolerance:

**AUTH_TIME_DRIFT_WINDOW**

Default value: +/- 1 time window (e.g. +/- 60 seconds)
Allowed range: 0 to 3 time windows

The parameter may be adjusted by the operator based on field conditions.


### 14.5 Packet Presence


When the AUTHENTICATED flag is set in the Header:
- An AUTH_TOKEN field (32 bits / 4 bytes) is included in the packet
  immediately preceding the Checksum.


### 14.6 Authorized Payload Encryption (Civil Protection Mode)


#### 14.6.1 Regulatory Context and Activation Policy
Under standard international amateur radio regulations (ITU Radio Regulations
RR 25.2, CEPT ECC recommendations, FCC Part 97.113), transmissions on amateur
bands shall be in plain language without cipher or obfuscation.
However, during formal disaster response, Search and Rescue (SAR) missions,
and declared emergencies where amateur operators provide auxiliary communications
to civil protection agencies (e.g., DSU, 112, FEMA, Red Cross), authorities
may require operational privacy:
- Protection of Personally Identifiable Information (GDPR / HIPAA compliance
  for patient triage, victim medical condition, casualty names).
- Operational security for law enforcement / SAR tactical deployment.

Accordingly, GHDSS supports an optional, authorized end-to-end payload encryption
mechanism, activated strictly upon explicit order from the competent emergency authority.


#### 14.6.2 End-to-End Encryption Scope (E2EE)
When authorized encryption is active:
- **Header in Plaintext:** The entire Layer 3 Header (Protocol ID, Version, Priority,
  Destination, Message Type, Flags, TTL, Message ID, Originator Station ID) and
  Addressing fields SHALL remain in plaintext.
- **Relay Transparency:** Intermediate digipeaters, relay stations, and mesh nodes
  can route, enforce TTL decrements, deduplicate, and verify message integrity
  without holding the cryptographic decryption key.
- **Payload Encryption:** Only the variable-length PAYLOAD octets (e.g., Position,
  Hazard details, triage text) SHALL be encrypted.


#### 14.6.3 Cryptographic Cipher and Zero-Overhead Stream Mode
To comply with the severe bandwidth limitations of emergency amateur radio modems,
payload encryption SHALL NOT introduce block cipher padding or message expansion:
- **Cipher Algorithm:** AES-128 in Counter Mode (AES-128-CTR) or ChaCha20 stream cipher.
- **Zero Padding Overhead:** Stream cipher encryption transforms an N-byte plaintext
  payload into an exact N-byte ciphertext payload (Delta = 0 bytes).
- **Deterministic Nonce / IV Generation:** To eliminate the overhead of transmitting
  a 12-byte or 16-byte Initialization Vector (IV) over the air, the Nonce SHALL be
  synthesized deterministically from existing packet header fields:
  Nonce = Message_ID (2B) || Originator_ID (7B) || UTC_Timestamp (4B) || Counter (3B)
  Because Message_ID and UTC_Timestamp are monotonically incremented and unique
  per originator, key-stream reuse is mathematically prevented.


#### 14.6.4 Key Management
- Cryptographic keys SHALL be 128-bit Pre-Shared Keys (PSK) distributed
  out-of-band by the Incident Net Control or civil authority before operational deployment.
- Stations lacking the incident PSK SHALL continue to relay the packet transparently,
  marking the payload locally as [ENCRYPTED - AUTHORIZED CIVIL PROTECTION].



---

## 15. Layer 3 Packet Specification



### 15.1 General Packet Architecture


Each GHDSS Layer 3 packet shall be encapsulated as payload within the
underlying Layer 2 modem broadcast service (e.g. VARA Broadcast FEC,

### AX.25 UI frame, or FreeDATA datagram).


A GHDSS packet consists of a contiguous binary octet stream organized as:

```text
+--------------+---------+--------------------+----------------+------------+----------+
| PROTOCOL_ID  | HEADER  | ADDRESSING INFO    | PAYLOAD        | AUTH_TOKEN | CHECKSUM |
| (2 B)        | (12 B)  | (0, 2, 4 or 7 B)   | (Variable)     | (0 or 4 B) | (2 B)    |
+--------------+---------+--------------------+----------------+------------+----------+
```

All multi-byte numeric fields shall be encoded in Network Byte Order
(Big-Endian, most significant byte first).


### 15.2 Protocol Identifier (2 Bytes)


Offset 0..1: 0x47 0x48 (ASCII "GH" - Global Hamradio), identifying the
payload as a valid GHDSS network packet.


### 15.3 Header Structure (Fixed 12 Bytes)


- Byte 2:
  - Bits 7..4: Protocol Version (4 bits, initial value = 1)
  - Bits 3..2: Priority (2 bits: 0: ROUTINE, 1: SAFETY, 2: URGENCY, 3: DISTRESS)
  - Bits 1..0: Destination Type (2 bits: 0: ALL_STATIONS, 1: INDIVIDUAL, 2: GROUP, 3: GRID)

- Byte 3:
  - Bits 7..4: Message Type (4 bits: 0: ALERT, 1: ACK, 2: OP_RESPONSE, 3: POSITION, 4: TELEMETRY, 5: TEST, 6: CONTROL_COMMAND)
  - Bit 3: ACK_REQUIRED flag (1 bit)
  - Bit 2: RELAY_ALLOWED flag (1 bit)
  - Bit 1: AUTHENTICATED flag (1 bit)
  - Bit 0: ENCRYPTED_PAYLOAD flag (1 bit: 0 = Plaintext, 1 = Encrypted Payload)

- Byte 4:
  - Bits 7..5: Time-To-Live / TTL (3 bits, range 0..7)
  - Bits 4..0: Reserved / Header Options (5 bits, default 0)

- Bytes 5..6: Message ID (16-bit unsigned integer, uint16_t, range 0..65535)

- Bytes 7..13: Originator Station ID (7 bytes / 56 bits total):
  - Bytes 7..12: Station Callsign (48 bits / Base40 encoded, up to 9 characters)
  - Byte 13:
    - Bits 7..3: SSID (5 bits, range 0..31)
    - Bits 2..0: Station Role (3 bits, from Appendix C)


### 15.4 Addressing Information Field (0, 2, 4 or 7 Bytes)


The presence and size depend strictly on Destination Type (Byte 2, Bits 1..0):

- Type 0 (ALL_STATIONS): 0 bytes (omitted).
- Type 1 (INDIVIDUAL):   7 bytes (Target Station ID in 48-bit Base40 + 5-bit SSID + 3 bits pad).
- Type 2 (GROUP):        2 bytes (Target GROUP_ID, uint16_t).
- Type 3 (GRID):         4 bytes (32-bit unsigned integer, uint32_t, Big-Endian).
                         Encodes an 8-character Maidenhead Locator (e.g. "KN34BK76"),
                         providing high-resolution geographic targeting (~400m x 500m).
                         Encodes Longitude index (0..43199) and Latitude index (0..43199)
                         into a single compact 32-bit integer (< 2^32).


### 15.5 Payload Specifications



#### 15.5.1 ALERT Payload (Message Type = 0)

- Byte 0: Hazard Code (6 bits, Appendix A) | Position Quality high bits (2 bits)
- Byte 1: Position Quality low bits (2 bits) | Position Age (3 bits) | Reserved (3 bits)
- Bytes 2..5: Latitude (int32_t, degrees * 100000)
- Bytes 6..9: Longitude (int32_t, degrees * 100000)
- Bytes 10..13: UTC Timestamp (uint32_t, Unix Epoch seconds)
- Byte 14: Detail Text Length (uint8_t, range 0..128)
- Bytes 15..15+N-1: Detail Text (ASCII string, N bytes; omitted if Length = 0)


#### 15.5.2 ACKNOWLEDGEMENT Payload (Message Type = 1)

- Bytes 0..6: Referenced Originator ID (7 bytes)
- Bytes 7..8: Referenced Message ID (uint16_t, 2 bytes)
- Byte 9: ACK Status / Result Code (uint8_t, 1 byte)


#### 15.5.3 OPERATIONAL_RESPONSE Payload (Message Type = 2)

- Bytes 0..6: Referenced Originator ID (7 bytes)
- Bytes 7..8: Referenced Message ID (uint16_t, 2 bytes)
- Byte 9: Response Code (6 bits, Appendix B) | Reserved (2 bits)
- Byte 10: Detail Text Length (uint8_t, range 0..128)
- Bytes 11..11+N-1: Detail Text (ASCII string, N bytes; omitted if Length = 0)


#### 15.5.4 CHECK_IN Payload (Message Type = 6 / Control Code 13)

- Byte 0: Station Role (3 bits) | Tactical Capability (4 bits) | Reserved (1 bit)
- Byte 1: Operational Status (uint8_t: 0: AVAILABLE, 1: EN_ROUTE, 2: ON_SCENE)
- Bytes 2..5: Current Latitude (int32_t, degrees * 100000)
- Bytes 6..9: Current Longitude (int32_t, degrees * 100000)


#### 15.5.5 NET_ROSTER_ECHO Payload (Message Type = 6 / Control Code 14)

Broadcast by a Control Station to confirm registered stations during automated roll-call:
- Byte 0: Current Frame Index (uint8_t: 1..255)
- Byte 1: Total Frames in Sequence (uint8_t: 1..255)
- Byte 2: Station Count in this Frame (uint8_t: N <= 16)
- Bytes 3 .. 3 + (7 * N) - 1: Array of N Station Records (7 bytes each):
  - Bytes 0..5: Base40-encoded Callsign (48 bits / 6 bytes)
  - Byte 6: Station SSID (5 bits: bits 7..3) | Station Role (3 bits: bits 2..0)


### 15.6 Authentication Field (0 or 4 Bytes)


Present only when AUTHENTICATED flag = 1 (Byte 3, Bit 1):
- Bytes 0..3: AUTH_TOKEN (32 bits, Ticket / OTP truncated hash).


### 15.7 Packet Checksum (2 Bytes)


The final 2 bytes contain a CRC-16-CCITT checksum (polynomial 0x1021, initial
value 0xFFFF) calculated across all bytes from Protocol Identifier through the
last byte of the payload or AUTH_TOKEN.


### 15.8 Packet Sizing and MTU Considerations



#### 15.8.1 Maximum Transmission Unit (MTU) Budget


The theoretical maximum GHDSS Layer 3 packet size is 170 bytes,
calculated for a worst-case scenario:
- Protocol Identifier: 2 bytes
- Fixed Header: 12 bytes
- Addressing (INDIVIDUAL destination): 7 bytes
- ALERT Payload with maximum Detail Text (128 bytes): 143 bytes
- Authentication Token (AUTH_TOKEN): 4 bytes
- Packet Checksum: 2 bytes
Total Maximum Length: 170 bytes.


#### 15.8.2 Typical Operational Packet Sizes


Routine and emergency packets remain highly compact:
- Minimal DISTRESS Alert (Position + Hazard, no text): 31 bytes
- Standard DISTRESS Alert (with typical 26-char text): 57 bytes
- ACKNOWLEDGEMENT (ACK): 26 bytes
- CHECK_IN Report: 30 bytes (unauthenticated) / 34 bytes (authenticated)
- OPERATIONAL_RESPONSE: 26 bytes (no text) / 50-70 bytes (with text)


#### 15.8.3 Layer 2 Modem Compatibility


A maximum packet length of 170 bytes ensures seamless operation
without fragmentation across target amateur radio modems:
- AX.25 Packet Radio: Comfortably below the standard 256-byte MTU limit.
- VARA HF / FreeDATA: Transmitted in a single broadcast burst (typically
  1.5 to 3.5 seconds on HF), minimizing channel occupancy and reducing
  vulnerability to ionospheric fading.


#### 15.8.4 Operational Payload Constraints and High-Volume Data Handoff


The GHDSS Layer 3 broadcast channel is engineered strictly as an
**Alerting, Signaling, and Channel Coordination Layer**.

Operational constraints:
- **Maximum Alert Packet:** Shall not exceed 170 bytes under any circumstance.
- **Text Brevity:** Detail text fields are limited to a maximum of 128 characters
  (recommended 26-50 characters) to ensure ultra-fast over-the-air transmission.
- **High-Volume Data Restriction:** Bulk operational traffic, such as casualty
  rosters, high-resolution imagery, damage assessment documents, or detailed ICS-213
  forms, SHALL NOT be transmitted across the GHDSS Layer 3 broadcast channel.
- **Layer 2 ARQ Handoff:** High-volume traffic SHALL be delegated to dedicated
  point-to-point ARQ modems (e.g., VARA ARQ Point-to-Point, Winlink session,
  or FreeDATA ARQ) via the DATA_SESSION_REQUEST signaling mechanism on assigned
  tactical frequencies.
- **Encryption Overhead Impact:** Because stream ciphers (AES-128-CTR / ChaCha20)
  exhibit zero length expansion, encrypted payloads occupy the exact same byte
  count as plaintext payloads, preserving all MTU budget guarantees.



---

## 16. Layer 2 Modem Convergence Sublayer



### 16.1 General


The Convergence Sublayer defines the interface rules for passing GHDSS
Layer 3 packets to and from supported Layer 2 modem technologies.


### 16.2 AX.25 Packet Radio Encapsulation


GHDSS packets transmitted over AX.25 Packet Radio (VHF/UHF 1200/9600 bps)
shall be encapsulated as follows:

- Frame Type: Unnumbered Information (UI) frame (Control byte = 0x03).
- Protocol Identifier (PID): 0xF0 (No Layer 3 protocol / raw data).
- Destination Address: "GHDSS-0" (or generic "CQ").
- Source Address: Station Originator Callsign and SSID.
- Payload: The complete contiguous GHDSS Layer 3 packet (Protocol ID through Checksum).
- Interface: Standard KISS TNC protocol (serial or TCP port 8001, e.g. Direwolf).
- FEC Enhancement: Where supported, FX.25 Reed-Solomon encapsulation
  SHOULD be enabled at the TNC level for forward error correction.


### 16.3 VARA HF / FM Encapsulation


GHDSS packets transmitted via the VARA modem shall use the VARA Broadcast
mode with native OFDM Forward Error Correction (FEC):

- Mode: Broadcast FEC (unproto / connectionless).
- Control Connection: Local TCP port 8300 (ASCII commands).
- Data Connection: Local TCP port 8301 (binary stream).
- Transmission Command: The packet is submitted via the VARA BROADCAST command.
- Channel Sensing Integration: The application MUST monitor asynchronous
  "BUSY ON" and "BUSY OFF" events on port 8300 to satisfy Section 8.3 (LBT).
- Signal Reporting: Upon packet reception, the accompanying SNR value
  reported by VARA SHOULD be logged in the Event Log.


### 16.4 FreeDATA and ARDOP Encapsulation


- FreeDATA: Encapsulated as a connectionless broadcast datagram via the
  FreeDATA TCP/WebSocket API.
- ARDOP: Transmitted via the ARDOP TNC interface in UNPROTO mode with
  maximum FEC redundancy.


### 16.5 LoRa Packet Framing and Physical Encapsulation


GHDSS Layer 3 packets transmitted across LoRa (Chirp Spread Spectrum)
transceivers (e.g. Semtech SX1262, SX1268, SX1276) shall be encapsulated
and modulated as follows:

- **Modulation & Framing:** Standard LoRa packet framing with explicit header
  mode (variable payload length enabled).
- **Physical Payload:** The complete GHDSS Layer 3 binary packet (starting with
  Protocol ID 0x47 0x48 through Checksum) is mapped directly into the LoRa FIFO
  payload buffer without additional framing overhead.
- **MTU Compatibility:** Because the native LoRa FIFO buffer capacity is 255
  octets and the maximum GHDSS Layer 3 packet length is strictly capped at 170
  octets, each GHDSS message SHALL be transmitted as a single, unfragmented
  physical radio burst.
- **Forward Error Correction (FEC):** Transceivers SHALL use a coding rate of
  at least 4/5 (CR 4/5); for emergency distress alerting over severe terrain,
  a coding rate of 4/7 or 4/8 SHOULD be configured.
- **Listen-Before-Talk (LBT) Integration:** The modem SHALL perform Channel
  Activity Detection (CAD) prior to transmission to satisfy Section 8.3 (Tier-1
  Channel Sensing). If CAD returns positive (preamble detected), the station
  SHALL back off randomly according to the GHDSS collision avoidance algorithm.
- **Frequency & Amateur Compliance:**
  - In amateur radio allocations (e.g. 70cm band, 430-440 MHz in IARU Region 1 /
    420-450 MHz in IARU Region 2), stations SHALL operate within permitted
    amateur power limits and frequency sub-bands. Legal station identification
    is inherently satisfied by the Base40 Originator Callsign embedded in every
    GHDSS header.
  - In license-exempt ISM allocations (868 MHz in Region 1 / 915 MHz in Region 2),
    transmission duty cycles and ERP limits prescribed by regional telecommunications
    authorities (CEPT/ETSI, FCC) SHALL be respected.
- **Recommended Default RF Parameters (Tactical Emergency Simplex):**
  - Bandwidth (BW): 125 kHz (or 250 kHz for faster channel clearing time).
  - Spreading Factor (SF): SF7 to SF9 (optimal balance between link margin,
    mountain penetration, and short time-on-air).
  - Preamble Length: 8 symbols (standard).



---

## APPENDIX A: HAZARD REGISTRY



### A.1 General


The GHDSS Hazard Registry defines the codes used
to classify incidents and emergency situations.

Hazard Codes are carried in DISTRESS messages.

Hazard Code Field Size:

6 bits

Supported Values:

0 - 63


### A.2 Initial Hazard Code Assignments


* **0 UNSPECIFIED**
* **1 FIRE**
* **2 FLOODING**
* **3 COLLISION**
* **4 GROUNDING**
* **5 CAPSIZING**
* **6 SINKING**
* **7 MEDICAL_EMERGENCY**
* **8 LOST_PERSON**
* **9 EVACUATION_REQUIRED**

* **10 EARTHQUAKE**
* **11 FLOOD**
* **12 LANDSLIDE**
* **13 SEVERE_STORM**
* **14 TORNADO**
* **15 HURRICANE**
* **16 TSUNAMI**
* **17 WILDFIRE**
* **18 AVALANCHE**
* **19 EXTREME_HEAT**

* **20 EXTREME_COLD**
* **21 BLIZZARD**
* **22 LIGHTNING_DAMAGE**
* **23 POWER_FAILURE**
* **24 COMMUNICATION_FAILURE**
* **25 INFRASTRUCTURE_FAILURE**
* **26 DAM_FAILURE**
* **27 CHEMICAL_LEAK**
* **28 GAS_LEAK**
* **29 RADIATION_HAZARD**

* **30 BIOLOGICAL_HAZARD**
* **31 TOXIC_CONTAMINATION**
* **32 VEHICLE_BREAKDOWN**
* **33 VEHICLE_ACCIDENT**
* **34 AIRCRAFT_INCIDENT**
* **35 AIRCRAFT_CRASH**
* **36 MARITIME_INCIDENT**
* **37 PERSON_OVERBOARD**
* **38 VESSEL_DISABLED**
* **39 BEACON_ACTIVATION**

* **40 PERSONAL_EMERGENCY**
* **41 MISSING_HIKER**
* **42 MOUNTAIN_RESCUE**
* **43 CAVE_RESCUE**
* **44 WATER_RESCUE**
* **45 SEARCH_AND_RESCUE**
* **46 SECURITY_THREAT**
* **47 ARMED_ATTACK**
* **48 PIRACY**
* **49 TERRORIST_THREAT**

* **50 CIVIL_UNREST**
* **51 EMERGENCY_SHELTER**
* **52 FOOD_SHORTAGE**
* **53 WATER_SHORTAGE**
* **54 FUEL_SHORTAGE**
* **55 MEDICAL_SUPPLIES_REQUIRED**
* **56 TECHNICAL_ASSISTANCE**
* **57 RELAY_REQUESTED**
* **58 POSITION_REPORT**
* **59 WEATHER_HAZARD**

* **60 UNKNOWN_DISTRESS**
* **61 TEST_MESSAGE**
* **62 OTHER**
* **63 RESERVED**


### A.3 Registry Maintenance


Hazard Code assignments may be extended or
reassigned by future protocol revisions
without requiring modification of the core
packet structure.


---

## APPENDIX B: RESPONSE REGISTRY



### B.1 General


The Response Registry defines operational
actions communicated through OPERATIONAL
RESPONSE messages.

Response Code Field Size:

6 bits

Supported Values:

0 - 63


### B.2 Initial Response Code Assignments


* **0 UNSPECIFIED**

* **1 ASSISTANCE_ACCEPTED**
* **2 ASSISTANCE_DECLINED**
* **3 SAR_ACTIVE**
* **4 TEAM_DISPATCHED**
* **5 TEAM_ARRIVED**

* **6 MEDICAL_SUPPORT_ENROUTE**
* **7 EVACUATION_STARTED**
* **8 EVACUATION_COMPLETED**
* **9 SEARCH_IN_PROGRESS**
* **10 SEARCH_COMPLETED**

* **11 INCIDENT_TRANSFERRED**
* **12 RELAY_ACTIVE**
* **13 GATEWAY_ACTIVE**
* **14 STANDBY**
* **15 INCIDENT_CLOSED**

* **16 RESOURCE_REQUESTED**
* **17 RESOURCE_ALLOCATED**
* **18 RESOURCE_DEPLOYED**
* **19 AIR_SUPPORT_REQUESTED**
* **20 AIR_SUPPORT_ACTIVE**

* **21 GROUND_TEAM_REQUESTED**
* **22 GROUND_TEAM_ACTIVE**
* **23 MARITIME_SUPPORT_REQUESTED**
* **24 MARITIME_SUPPORT_ACTIVE**
* **25 MEDICAL_TEAM_REQUESTED**

* **26 MEDICAL_TEAM_ACTIVE**
* **27 EVACUATION_DELAYED**
* **28 ACCESS_BLOCKED**
* **29 COMMUNICATIONS_ESTABLISHED**
* **30 COMMUNICATIONS_LOST**

* **31 POSITION_CONFIRMED**
* **32 INCIDENT_CONFIRMED**
* **33 INCIDENT_UNCONFIRMED**
* **34 SECONDARY_INCIDENT**
* **35 ESCALATION_REQUESTED**

* **36 ESCALATED**
* **37 MUTUAL_AID_REQUESTED**
* **38 MUTUAL_AID_ACTIVE**
* **39 REMOTE_MONITORING_ACTIVE**
* **40 SITUATION_STABLE**

* **41 SITUATION_DETERIORATING**
* **42 SITUATION_IMPROVING**
* **43 VICTIM_LOCATED**
* **44 VICTIM_RECOVERED**
* **45 TRANSPORT_IN_PROGRESS**

* **46 TRANSPORT_COMPLETED**
* **47 FIELD_HOSPITAL_ACTIVE**
* **48 COMMAND_POST_ACTIVE**
* **49 COMMAND_POST_RELOCATED**
* **50 NETWORK_CONGESTED**

* **51 FREQUENCY_CHANGE_REQUIRED**
* **52 ALTERNATE_ROUTE_ACTIVE**
* **53 WEATHER_RESTRICTION**
* **54 SECURITY_RESTRICTION**
* **55 TRAINING_EXERCISE**

* **56 EXERCISE_CONTROLLER**
* **57 EXERCISE_COMPLETE**
* **58 AFTER_ACTION_REVIEW**
* **59 STATUS_UPDATE**

* **60 OTHER**
* **61 RESERVED**
* **62 RESERVED**
* **63 RESERVED**


### B.3 Registry Maintenance


Response Codes may be modified by future
specification revisions.


---

## APPENDIX C: STATION ROLE REGISTRY



### C.1 General


The Station Role Registry defines operational
roles within the GHDSS network.

Station Role Field Size:

3 bits

Supported Values:

0 - 7


### C.2 Initial Assignments


* **0 OTHER**

General-purpose amateur radio station.

* **1 SAR**

Search and Rescue station.

* **2 EMCOMM**

Emergency Communications station.

* **3 GATEWAY**

Station capable of forwarding traffic
between networks, systems or modes.

* **4 CONTROL**

Network Control Station responsible for
coordination and traffic management.

* **5 RELAY**

Autonomous infrastructure digipeater or tactical line-of-sight
relay station (typically operating as Class 1R).

* **6 RESERVED**
* **7 RESERVED**


### C.3 Default Role


Stations shall default to:

**OTHER**



---

## APPENDIX D: RECOMMENDED DEFAULT CONFIGURATION


* **ACK_WAIT_TIMER**: 60 s

* **DISTRESS_RETRY_COUNT**: 5

* **DISTRESS_RETRY_INTERVAL**: 60 s

* **MESSAGE_RETRY_COUNT**: 3

* **MESSAGE_RETRY_INTERVAL**: 60 s

* **DUPLICATE_CACHE_MINIMUM**: 24 h

* **DUPLICATE_CACHE_RECOMMENDED**: 72 h

* **DISTRESS_TTL**: 3

* **URGENCY_TTL**: 2

* **SAFETY_TTL**: 2

* **ROUTINE_TTL**: 1




---

## APPENDIX E: EVENT LOG FORMAT



### E.1 Purpose


The Event Log provides a human-readable
record of protocol activity.

The Event Log is intended for:

- incident analysis
- interoperability testing
- training exercises
- after-action review
- troubleshooting


### E.2 Minimum Logged Fields


* **UTC_TIMESTAMP**

* **EVENT_TYPE**

* **MESSAGE_TYPE**

* **MESSAGE_ID**

* **ORIGINATOR_ID**

* **RESULT**


### E.3 Recommended Fields


* **REMOTE_STATION**

* **HAZARD_CODE**

* **RESPONSE_CODE**

* **LATITUDE**

* **LONGITUDE**

* **SIGNAL_INFORMATION**

* **DETAIL_TEXT**


### E.4 Example


2026-11-19T10:15:07Z
TX_DISTRESS
MID=48125
ORIGINATOR=YO3HJV-7
RESULT=SENT

2026-11-19T10:15:15Z
RX_ACK
MID=48125
ORIGINATOR=YO3HJV-7
REMOTE=YO3SAR-1
RESULT=VALID

2026-11-19T10:17:02Z
RX_OPERATIONAL_RESPONSE
MID=48125
RESPONSE=SAR_ACTIVE
REMOTE=YO3SAR-1


### E.5 CSV Export


Implementations should support CSV export.

Recommended CSV fields:

UTC_TIMESTAMP,
EVENT_TYPE,
MESSAGE_TYPE,
MESSAGE_ID,
ORIGINATOR_ID,
REMOTE_STATION,
* **RESULT**




---

## APPENDIX F: CONTACT METHOD REGISTRY



### F.1 General


The Contact Method Registry defines communication
methods supported by GHDSS stations.

Contact Method Field Size:

4 bits

Supported Values:

0 - 15


### F.2 Initial Assignments


* **0 UNSPECIFIED**

Voice Communications

* **1 VOICE_USB**
* **2 VOICE_LSB**
* **3 VOICE_FM**
* **4 VOICE_AM**

Morse Communications

* **5 CW**

Digital Communications

* **6 VARA_HF**
* **7 ARDOP**
* **8 FREEDATA**
* **9 JS8CALL**

* **10 PACKET**
* **11 APRS**
* **12 WINLINK**

* **13 DIGITAL_GENERIC**

* **14 RESERVED**
* **15 RESERVED**


### F.3 Contact Channel Definition


A Contact Channel is defined by:

- Contact Method
- Frequency

A station may define up to four Contact Channels.


### F.4 Contact Channel Priority


Contact Channels shall be ordered by priority.

Channel 1 has highest priority.

Channel 4 has lowest priority.


### F.5 Example


Channel 1

Method:
VOICE_USB

Frequency:
7110 kHz

Channel 2

Method:
VARA_HF

Frequency:
7105 kHz

Channel 3

Method:
JS8CALL

Frequency:
7078 kHz

Channel 4

Method:
FREEDATA

Frequency:
7095 kHz


### F.6 Registry Maintenance


Additional Contact Methods may be assigned
by future revisions of the specification.

Existing assignments should not be modified
unless required for compatibility reasons.





---

## APPENDIX G


AUTOMATION AND CONTROL MESSAGE REGISTRY


### G.1 General


The Automation and Control Message Registry
defines standardized coordination, control
and traffic management messages supported
by GHDSS.

Automation and Control Message Field Size:

4 bits

Supported Values:

0 - 15


### G.2 Initial Assignments


* **0 CONTACT_REQUEST**

Used to request direct communication.

* **1 FREQUENCY_CHANGE_REQUEST**

Used to request migration to a different
operating frequency.

* **2 MODE_CHANGE_REQUEST**

Used to request migration to a different
Contact Method.

* **3 DATA_SESSION_REQUEST**

Used to request establishment of a data
communication session.

* **4 POSITION_REQUEST**

Used to request the current position.

* **5 POSITION_TRACKING_REQUEST**

Used to request periodic position reports.

* **6 TRACKING_TERMINATION_REQUEST**

Used to terminate a tracking session.

* **7 STATUS_REQUEST**

Used to request operational status
information.

* **8 HOLD_TRAFFIC**

Used to suppress lower-priority traffic.

* **9 RESUME_TRAFFIC**

Used to cancel a previously established
traffic hold condition.

* **10 STAND_BY_REQUEST**

Used to request standby availability.

* **11 INCIDENT_NET_CONTROL**

Used to announce that a station has assumed
incident coordination responsibilities.

The message may contain:

- Control Station Identifier
- Incident Reference
- Optional Operating Frequency
- Optional Contact Method

Upon reception of a valid INCIDENT_NET_CONTROL
message, stations should direct operational
traffic related to the incident through the
designated Control Station whenever practical.


* **12 PRIORITY_TRAFFIC_NOTICE**
Used to inform stations that priority traffic
is currently being handled on the channel.

This message does not impose transmission
restrictions by itself.

If traffic restrictions are required,
HOLD_TRAFFIC shall be used.

* **13 CHECK_IN**
Used by a field station to announce its initial presence,
operational status, tactical capability, and GPS position
upon entering the incident network.

* **14 NET_ROSTER_ECHO**
Used by a Control Station to broadcast a paginated confirmation
frame containing an array of registered Station IDs during an
automated roll-call or incident check-in. Field stations unconfirmed
across all frames of the sequence SHALL retransmit a CHECK_IN packet
following sequence completion and guard delay.

* **15 RESERVED**


### G.3 Traffic Control Parameters


Traffic Control Messages may contain a
Minimum Allowed Priority parameter.

Defined values:

* **0 ROUTINE**
* **1 SAFETY**
* **2 URGENCY**
* **3 DISTRESS**

Example:

MINIMUM_PRIORITY = SAFETY

Allowed:

DISTRESS
URGENCY
SAFETY

Suppressed:

ROUTINE


### G.4 Target Types


Automation and Control Messages may be
addressed to:

- INDIVIDUAL
- GROUP
- GRID
- ALL_STATIONS


### G.5 Registry Maintenance


New Automation and Control Message assignments
may be added by future revisions of the
specification.

Existing assignments should remain stable
to preserve interoperability.



---

## APPENDIX H: TACTICAL CAPABILITY REGISTRY



### H.1 General


The Tactical Capability Registry defines operational specialties
for stations participating in SAR, EMCOMM and incident networks.

Tactical Capability Field Size:

4 bits

Supported Values:

0 - 15


### H.2 Initial Tactical Capability Assignments


* **0 UNSPECIFIED / GENERAL_SAR**
   General search, rescue, or emergency communications asset.

* **1 GROUND_SEARCH**
   Ground search team (foot, tracking, grid sweep).

* **2 K9_SEARCH**
   Canine search and rescue team.

* **3 MOUNTAIN_RESCUE**
   Specialized alpine / wilderness / high-angle rescue unit.

* **4 WATER_RESCUE**
   Dive, swiftwater, or maritime / boat rescue unit.

* **5 AIR_SUPPORT**
   Aviation asset (drone / UAV, light aircraft, helicopter).

* **6 MEDICAL_FIRST_AID**
   Field medical team, triage, ambulance, or paramedic unit.

* **7 EVACUATION_TRANSPORT**
   Victim evacuation and logistical transport.

* **8 SHELTER_LOGISTICS**
   Emergency shelter management, food, water, supplies.

* **9 REPEATER_GATEWAY**
   Radio infrastructure relay, tactical cross-band, or IP gateway.

* **10 TECHNICAL_SUPPORT**
   Power systems, RF engineering, IT, field mechanics.

* **11 COMMAND_POST**
   Field incident command post or mobile operations center.

* **12 RESERVED**
* **13 RESERVED**
* **14 RESERVED**
* **15 RESERVED**


### H.3 Registry Maintenance


Tactical Capability assignments may be extended or updated
by future protocol revisions without requiring changes to
the core packet structure.



---

## APPENDIX I: CANONICAL ALGORITHMS



### I.1 Callsign Base40 Encoding and Decoding



#### I.1.1 Character Mapping Table


Value | Character     | Value | Character     | Value | Character
------+---------------+-------+---------------+-------+----------
 0    | ' ' (SPACE)   | 13    | 'C'           | 26    | 'P'
 1..10| '0'..'9'      | 14    | 'D'           | 27    | 'Q'
 11   | 'A'           | 15    | 'E'           | 28    | 'R'
 12   | 'B'           | ...   | ...           | 37    | '/'
                      |       |               | 38..39| RESERVED


#### I.1.2 Encoding Algorithm (Callsign to 48-bit / 6-byte binary)


// Input: Callsign string up to 9 characters (e.g. "YO3HJV/P")
// Output: 6 bytes (three 16-bit big-endian words)

```c
function encode_base40(string callsign) -> bytes[6]:
    // Right-pad callsign with spaces up to 9 characters
    string s = pad_right(uppercase(callsign), 9, ' ')
    bytes output[6]
    
    for triplet = 0 to 2:
        c0 = char_to_base40(s[triplet * 3 + 0])
        c1 = char_to_base40(s[triplet * 3 + 1])
        c2 = char_to_base40(s[triplet * 3 + 2])
        
        uint16_t word = (c0 * 1600) + (c1 * 40) + c2
        
        output[triplet * 2 + 0] = (word >> 8) & 0xFF
        output[triplet * 2 + 1] = word & 0xFF
        
    return output
```


#### I.1.3 Decoding Algorithm (48-bit binary to Callsign)


```c
function decode_base40(bytes[6] data) -> string:
    string result = ""
    for triplet = 0 to 2:
        uint16_t word = (data[triplet * 2] << 8) | data[triplet * 2 + 1]
        
        c2 = word % 40; word = word / 40
        c1 = word % 40; word = word / 40
        c0 = word % 40
        
        result += base40_to_char(c0)
        result += base40_to_char(c1)
        result += base40_to_char(c2)
        
    return trim_right(result, ' ')
```



### I.2 Maidenhead 8-Character Locator to uint32_t Conversion



#### I.2.1 Encoding Algorithm


// Input: 8-character locator: F1 F2 S1 S2 s1 s2 e1 e2 (e.g. "KN34BK76")
// Output: uint32_t binary integer (0..1866239999)

```c
function encode_maidenhead_8(string grid) -> uint32_t:
    // Longitude components
    lon_field = uppercase(grid[0]) - 'A'    // 0..17
    lon_sq    = grid[2] - '0'               // 0..9
    lon_sub   = uppercase(grid[4]) - 'A'    // 0..23
    lon_ext   = grid[6] - '0'               // 0..9
    
    uint32_t lon_index = (lon_field * 2400) + (lon_sq * 240) + (lon_sub * 10) + lon_ext
    
    // Latitude components
    lat_field = uppercase(grid[1]) - 'A'    // 0..17
    lat_sq    = grid[3] - '0'               // 0..9
    lat_sub   = uppercase(grid[5]) - 'A'    // 0..23
    lat_ext   = grid[7] - '0'               // 0..9
    
    uint32_t lat_index = (lat_field * 2400) + (lat_sq * 240) + (lat_sub * 10) + lat_ext
    
    // Combine into single 32-bit unsigned integer (< 2^32)
    return (lon_index * 43200) + lat_index
```


#### I.2.2 Decoding Algorithm


```c
function decode_maidenhead_8(uint32_t code) -> string:
    uint32_t lat_index = code % 43200
    uint32_t lon_index = code / 43200
    
    lat_ext = lat_index % 10; lat_index /= 10
    lat_sub = lat_index % 24; lat_index /= 24
    lat_sq  = lat_index % 10; lat_index /= 10
    lat_fld = lat_index
    
    lon_ext = lon_index % 10; lon_index /= 10
    lon_sub = lon_index % 24; lon_index /= 24
    lon_sq  = lon_index % 10; lon_index /= 10
    lon_fld = lon_index
    
    char grid[8]
    grid[0] = 'A' + lon_fld; grid[1] = 'A' + lat_fld
    grid[2] = '0' + lon_sq;  grid[3] = '0' + lat_sq
    grid[4] = 'A' + lon_sub; grid[5] = 'A' + lat_sub
    grid[6] = '0' + lon_ext; grid[7] = '0' + lat_ext
    return string(grid)
```



### I.3 Canonical CRC-16-CCITT Implementation


Parameters:
- Polynomial: 0x1021 (x^16 + x^12 + x^5 + 1)
- Initial Value: 0xFFFF
- RefIn: False (MSB first)
- RefOut: False (MSB first)
- Final XOR: 0x0000

```c
uint16_t crc16_ccitt(const uint8_t *data, size_t length) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= ((uint16_t)data[i] << 8);
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}
```




---

## APPENDIX J: OFFICIAL TEST VECTORS


The following canonical test vectors provide an authoritative reference
for verifying encoder and decoder implementations. All byte sequences
are shown in hexadecimal format in transmission order.


### J.1 Test Vector 1: Minimal DISTRESS ALERT (31 Bytes)


Operational Scenario:
Station YO3HJV-7 (Role: OTHER) originates an emergency alert for a
medical emergency (Hazard: 7). GPS position is 44.42677° N, 26.10254° E,
timestamp 1732014000 (2024-11-19T10:20:00Z). Priority is DISTRESS,
destination is ALL_STATIONS, ACK_REQUIRED is set, TTL is 3.

Dissected Packet Fields:
- Protocol ID    : 47 48 ("GH")
- Header Byte 2  : 1C (Version=1, Priority=3 [DISTRESS], Dest=0 [ALL_STATIONS])
- Header Byte 3  : 08 (Type=0 [ALERT], Flags=ACK_REQUIRED [0x08])
- Header Byte 4  : 60 (TTL=3)
- Message ID     : 03 E9 (1001)
- Originator ID  : DE AC 73 C0 00 00 38 (Callsign "YO3HJV   " in Base40, SSID=7, Role=0)
- Addressing     : (Omitted, 0 bytes)
- Hazard Code    : 1C (Hazard=7 [MEDICAL_EMERGENCY], Quality_hi=0)
- Quality & Age  : C0 (Quality_lo=3 [GNSS], Age=0 [CURRENT], Flags=0)
- Latitude       : 00 43 CA 35 (+4442677 -> 44.42677°)
- Longitude      : 00 27 D4 4E (+2610254 -> 26.10254°)
- UTC Timestamp  : 67 3C 6F B0 (1732014000)
- Text Length    : 00 (0 bytes text)
- Checksum       : 1C 22 (CRC-16-CCITT)

Complete Hex Stream (31 Bytes):
```hex
47 48 1C 08 60 03 E9 DE AC 73 C0 00 00 38 1C C0 00 43 CA 35 00 27 D4 4E 67 3C 6F B0 00 1C 22
```



### J.2 Test Vector 2: ACKNOWLEDGEMENT Packet (26 Bytes)


Operational Scenario:
Station YO3SAR-1 (Role: SAR) confirms receipt of the above DISTRESS alert
(Originator: YO3HJV-7, Message ID: 1001). The ACK message ID is 2005.

Dissected Packet Fields:
- Protocol ID    : 47 48 ("GH")
- Header Byte 2  : 1C (Version=1, Priority=3 [DISTRESS], Dest=0)
- Header Byte 3  : 10 (Type=1 [ACKNOWLEDGEMENT], Flags=0)
- Header Byte 4  : 20 (TTL=1)
- Message ID     : 07 D5 (2005)
- Originator ID  : DE AC B7 14 00 00 09 (Callsign "YO3SAR   " in Base40, SSID=1, Role=1 [SAR])
- Ref Originator : DE AC 73 C0 00 00 38 (YO3HJV-7)
- Ref Message ID : 03 E9 (1001)
- Result Code    : 00 (VALID / ACCEPTED)
- Checksum       : BA F7 (CRC-16-CCITT)

Complete Hex Stream (26 Bytes):
```hex
47 48 1C 10 20 07 D5 DE AC B7 14 00 00 09 DE AC 73 C0 00 00 38 03 E9 00 BA F7
```



### J.3 Test Vector 3: CHECK_IN with Tactical Capability (26 Bytes)


Operational Scenario:
Station YO3SAR-1 (Role: SAR) announces its readiness upon entering the net
with Tactical Capability = 6 (MEDICAL_FIRST_AID) at position 44.42677° N, 26.10254° E.

Complete Hex Stream (26 Bytes):
```hex
47 48 10 60 20 0B B9 DE AC B7 14 00 00 09 2C 00 00 43 CA 35 00 27 D4 4E 71 BB
```

