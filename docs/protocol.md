# Unit RFID-UHF — Common Control Commands

> **Source:** Unit-RFID-UHF-Protocol-EN.pdf  
> **Target chip:** M100 / QM100  
> **Encoding:** All values hexadecimal unless noted otherwise.

---

## Table of Contents

1. [Firmware Command Overview](#1-firmware-command-overview)
   - 1.1 [Command Frame Format](#11-command-frame-format)
   - 1.2 [Command Type](#12-command-type)
2. [Common Command Definitions](#2-common-command-definitions)
   - 2.1 [Get Reader Module Information](#21-get-reader-module-information)
   - 2.2 [Single Polling Command](#22-single-polling-command)
   - 2.3 [Multiple Polling Command](#23-multiple-polling-command)
   - 2.4 [Stop Multiple Polling Command](#24-stop-multiple-polling-command)
   - 2.5 [Set Select Parameter Command](#25-set-select-parameter-command)
   - 2.6 [Get Select Parameter](#26-get-select-parameter)
   - 2.7 [Set Select Mode](#27-set-select-mode)
   - 2.8 [Read Tag Memory Area](#28-read-tag-memory-area)
   - 2.9 [Write Tag Memory Area](#29-write-tag-memory-area)
   - 2.10 [Lock Tag Data Storage Area](#210-lock-tag-data-storage-area)
   - 2.11 [Kill Tag](#211-kill-tag)
   - 2.12 [Set Communication Baud Rate](#212-set-communication-baud-rate)
   - 2.13 [Get Query Parameters](#213-get-query-parameters)
   - 2.14 [Set Query Parameters](#214-set-query-parameters)
   - 2.15 [Set Operating Region](#215-set-operating-region)
   - 2.16 [Get Operating Region](#216-get-operating-region)
   - 2.17 [Set Operating Channel](#217-set-operating-channel)
   - 2.18 [Get Operating Channel](#218-get-operating-channel)
   - 2.19 [Set Automatic Frequency Hopping](#219-set-automatic-frequency-hopping)
   - 2.20 [Insert Operating Channel](#220-insert-operating-channel)
   - 2.21 [Get Transmit Power](#221-get-transmit-power)
   - 2.22 [Set Transmit Power](#222-set-transmit-power)
   - 2.23 [Set Continuous Carrier Transmission](#223-set-continuous-carrier-transmission)
   - 2.24 [Get Receiver Demodulator Parameters](#224-get-receiver-demodulator-parameters)
   - 2.25 [Set Receiver Demodulator Parameters](#225-set-receiver-demodulator-parameters)
   - 2.26 [Test RF Input Blocking Signal](#226-test-rf-input-blocking-signal)
   - 2.27 [Test Channel RSSI](#227-test-channel-rssi)
   - 2.28 [Control IO Port](#228-control-io-port)
   - 2.29 [Module Sleep](#229-module-sleep)
   - 2.30 [Module Idle Sleep Time](#230-module-idle-sleep-time)
   - 2.31 [IDLE Mode](#231-idle-mode)
   - 2.32 [NXP ReadProtect / Reset ReadProtect](#232-nxp-readprotect--reset-readprotect)
   - 2.33 [NXP Change EAS Command](#233-nxp-change-eas-command)
   - 2.34 [NXP EAS_Alarm Command](#234-nxp-eas_alarm-command)
   - 2.35 [NXP ChangeConfig Command](#235-nxp-changeconfig-command)
   - 2.36 [Impinj Monza QT Command](#236-impinj-monza-qt-command)
   - 2.37 [BlockPermalock Command](#237-blockpermalock-command)
3. [Command Summary](#3-command-summary)
4. [Error Code Summary](#4-error-code-summary)

---

## 1. Firmware Command Overview

### 1.1 Command Frame Format

The firmware command consists of the following fields, all in hexadecimal:

| Field | Description |
|-------|-------------|
| **Header** | Always `0xBB` |
| **Type** | Frame type (see §1.2) |
| **Command** | Command code |
| **PL (MSB)** | Parameter Length, most-significant byte |
| **PL (LSB)** | Parameter Length, least-significant byte |
| **Parameter(s)** | Command-specific data |
| **Checksum** | Cumulative sum of bytes from Type through last Parameter; only LSB retained |
| **End** | Always `0x7E` |

**Example** — Set Operating Region (China 900 MHz):

```
BB 00 07 00 01 01 09 7E
```

| Header | Type | Command | PL(MSB) | PL(LSB) | Parameter | Checksum | End |
|--------|------|---------|---------|---------|-----------|----------|-----|
| `BB`   | `00` | `07`    | `00`    | `01`    | `01`      | `09`     | `7E` |

> **Checksum formula:** `sum(Type, Command, PL_MSB, PL_LSB, Parameters...) & 0xFF`

---

### 1.2 Command Type

| Type | Direction | Description |
|------|-----------|-------------|
| `0x00` | Host → M100 | **Command Frame** — sent from host to chip |
| `0x01` | M100 → Host | **Response Frame** — chip confirms command execution |
| `0x02` | M100 → Host | **Notification Frame** — chip autonomously sends tag data |

**Notes:**
- Every Command Frame has a corresponding Response Frame indicating success/failure.
- Single and Multiple Polling commands also generate Notification Frames. The number of Notification Frames equals the number of tags detected.

---

## 2. Common Command Definitions

---

### 2.1 Get Reader Module Information

**Command:** `0x03`

#### 2.1.1 Command Frame

| Sub-parameter | Value | Description |
|---------------|-------|-------------|
| `0x00` | Hardware Version | |
| `0x01` | Software Version | |
| `0x02` | Manufacturer | |

**Hardware Version request:**
```
BB 00 03 00 01 00 04 7E
```

**Software Version request:**
```
BB 00 03 00 01 01 05 7E
```

**Manufacturer request:**
```
BB 00 03 00 01 02 06 7E
```

#### 2.1.2 Response Frame

- **Type:** `0x01`
- **Command:** `0x03`
- **Information encoding:** ASCII

**Example — Hardware Version "M100 V1.00":**

| Char | M    | 1    | 0    | 0    | (sp) | V    | 1    | .    | 0    | 0    |
|------|------|------|------|------|------|------|------|------|------|------|
| Hex  | `4D` | `31` | `30` | `30` | `20` | `56` | `31` | `2E` | `30` | `30` |

```
BB 01 03 00 0B 00 4D 31 30 30 20 56 31 2E 30 30 22 7E
```

---

### 2.2 Single Polling Command

Reads one or more tags in the field; returns RSSI, PC, EPC, and CRC per tag.

#### 2.2.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x22`  
- **Parameter Length:** `0x0000`  
- **Checksum:** `0x22`

```
BB 00 22 00 00 22 7E
```

#### 2.2.2 Notification Frame (tag found)

- **Type:** `0x02`
- **Command:** `0x22`
- **Parameter Length:** `0x0011` (17 bytes: 1 RSSI + 2 PC + 12 EPC + 2 CRC)

| Field | Example | Notes |
|-------|---------|-------|
| RSSI | `0xC9` | Signed hex, unit dBm. `0xC9` = −55 dBm |
| PC | `0x3400` | Protocol Control word |
| EPC | `0x30751FEB705C5904E3D50D70` | 12 bytes |
| CRC | `0x3A76` | |

```
BB 02 22 00 11 C9 34 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 3A 76 EF 7E
```

> **RSSI note:** Value is a signed hexadecimal byte at the chip input, excluding antenna gain or coupler attenuation.

#### 2.2.3 Notification Frame (no tag / CRC error)

- **Type:** `0x01`  
- **Command:** `0xFF`  
- **Error Code:** `0x15`

```
BB 01 FF 00 01 15 16 7E
```

---

### 2.3 Multiple Polling Command

Continuously polls for tags up to a configurable count (0–65535).

#### 2.3.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x27`  
- **Parameter Length:** `0x0003`  
- Fields: `Reserved(1)` + `CNT_MSB` + `CNT_LSB`

**Example — 10,000 polls:**

| Header | Type | Command | PL(MSB) | PL(LSB) | Reserved | CNT(MSB) | CNT(LSB) | Checksum | End |
|--------|------|---------|---------|---------|----------|----------|----------|----------|-----|
| `BB`   | `00` | `27`    | `00`    | `03`    | `22`     | `27`     | `10`     | `83`     | `7E` |

```
BB 00 27 00 03 22 27 10 83 7E
```

#### 2.3.2 Notification Frame (tag found)

Same structure as §2.2.2 but Command = `0x27`.

```
BB 02 27 00 11 C9 34 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 3A 76 EF 7E
```

#### 2.3.3 Notification Frame (no tag / CRC error)

Same as §2.2.3: error code `0x15`.

```
BB 01 FF 00 01 15 16 7E
```

---

### 2.4 Stop Multiple Polling Command

Immediately stops an ongoing multiple polling operation (not a pause).

#### 2.4.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x28`  
- **Parameter Length:** `0x0000`  
- **Checksum:** `0x28`

```
BB 00 28 00 00 28 7E
```

#### 2.4.2 Response Frame (success)

- **Type:** `0x01`, **Command:** `0x28`, **Parameter:** `0x00`

```
BB 01 28 00 01 00 2A 7E
```

---

### 2.5 Set Select Parameter Command

Sets Select parameters and automatically sets Select Mode to `0x02` (send Select before all operations except polling).

#### 2.5.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x0C`  
- **Parameter Length:** `0x0013` (19 bytes)

| Field | Size | Description |
|-------|------|-------------|
| SelParam | 1 byte | Packed: `Target[7:5]` + `Action[4:2]` + `MemBank[1:0]` |
| Ptr | 4 bytes | Bit address offset into the memory bank |
| MaskLen | 1 byte | Mask length in bits |
| Truncate | 1 byte | `0x00` = disabled, `0x80` = enabled |
| Mask | up to 12 bytes | EPC mask value |

**SelParam MemBank encoding:**

| Code | Memory Bank |
|------|-------------|
| `2'b00` | RFU |
| `2'b01` | EPC |
| `2'b10` | TID |
| `2'b11` | User |

**Example** — Select by EPC (MemBank=EPC, Ptr=0x20, MaskLen=96 bits):

```
BB 00 0C 00 13 01 00 00 00 20 60 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 AD 7E
```

> **Note:** When MaskLen > 80 bits (5 words), the command first resets all tags to `Inventoried=A` / `SL=~SL` before applying the Action.

#### 2.5.2 Response Frame (success)

```
BB 01 0C 00 01 00 0E 7E
```

---

### 2.6 Get Select Parameter

Returns the currently stored Select parameters.

#### 2.6.1 Command Frame

```
BB 00 0B 00 00 0B 7E
```

#### 2.6.2 Response Frame

Same field layout as §2.5.1 Command Frame but Type=`0x01`, Command=`0x0B`.

```
BB 01 0B 00 13 01 00 00 00 20 60 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 AD 7E
```

---

### 2.7 Set Select Mode

Controls when the Select command is sent relative to tag operations.

#### 2.7.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x12`  
- **Parameter Length:** `0x0001`

| Mode | Description |
|------|-------------|
| `0x00` | Send Select before **every** tag operation |
| `0x01` | **Never** send Select |
| `0x02` | Send Select before non-inventory operations only (Read, Write, Lock, Kill) |

**Example — disable Select (mode 0x01):**

```
BB 00 12 00 01 01 14 7E
```

#### 2.7.2 Response Frame (success)

```
BB 01 0C 00 01 00 0E 7E
```

---

### 2.8 Read Tag Memory Area

Reads data from a specified memory bank address. Address offset (SA) and data length (DL) are in **words** (2 bytes / 16 bits each). Must pre-configure Select parameters. If Access Password is all zeros, the Access command is not sent.

#### 2.8.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x39`  
- **Parameter Length:** `0x0009`

| Field | Size | Description |
|-------|------|-------------|
| Access Password (AP) | 4 bytes | Tag access password |
| MemBank | 1 byte | `0x00`=RFU, `0x01`=EPC, `0x02`=TID, `0x03`=User |
| SA (MSB+LSB) | 2 bytes | Start address offset in words |
| DL (MSB+LSB) | 2 bytes | Data length in words |

**Example** — Read 2 words from User memory (MemBank=0x03, SA=0, DL=2):

```
BB 00 39 00 09 00 00 FF FF 03 00 00 00 02 45 7E
```

#### 2.8.2 Response Frame (success)

- **Type:** `0x01`, **Command:** `0x39`
- Fields: `UL` (EPC length) + `PC` + `EPC` + `Data`

```
BB 01 39 00 13 0E 34 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 12 34 56 78 B0 7E
```

#### Error Responses

| Error | Code | Trigger |
|-------|------|---------|
| Tag not found / EPC mismatch | `0x09` | `BB 01 FF 00 01 09 0A 7E` |
| Incorrect Access Password | `0x16` | Returns code + PC + EPC |
| EPC Gen2 tag error | `0xA0 \| error_code` | E.g., Memory Overrun → `0xA3` |

---

### 2.9 Write Tag Memory Area

Writes data to a specified memory bank address. SA and DL in words. Max write length: 32 words (64 bytes / 512 bits).

#### 2.9.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x49`  
- **Parameter Length:** `0x000D`

| Field | Size | Description |
|-------|------|-------------|
| Access Password (AP) | 4 bytes | |
| MemBank | 1 byte | Memory bank selector |
| SA (MSB+LSB) | 2 bytes | Start address in words |
| DL (MSB+LSB) | 2 bytes | Data length in words |
| DT | variable | Data to write |

**Example** — Write `0x12345678` to User memory (SA=0, DL=2):

```
BB 00 49 00 0D 00 00 FF FF 03 00 00 00 02 12 34 56 78 6D 7E
```

#### 2.9.2 Response Frame (success)

```
BB 01 49 00 10 0E 34 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 A9 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| Tag not found / EPC mismatch | `0x10` |
| Incorrect Access Password | `0x16` (+ PC + EPC) |
| EPC Gen2 tag error | `0xB0 \| error_code` |

---

### 2.10 Lock Tag Data Storage Area

Locks or unlocks a tag's memory area. Precede with Select parameter setup.

#### 2.10.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x82`  
- **Parameter Length:** `0x0007`

| Field | Size | Description |
|-------|------|-------------|
| Access Password (AP) | 4 bytes | |
| LD | 3 bytes | Lock operation payload (upper 4 bits reserved; 20-bit payload = 10-bit Mask + 10-bit Action) |

**LD Bit Layout (20 bits, MSB→LSB):**

```
[Kill_Mask(2)] [Access_Mask(2)] [EPC_Mask(2)] [TID_Mask(2)] [User_Mask(2)]
[Kill_Act(2)]  [Access_Act(2)]  [EPC_Act(2)]  [TID_Act(2)]  [User_Act(2)]
```

**Action encoding per memory area:**

| Bits | Meaning |
|------|---------|
| `00` | Open (read/write without access password) |
| `01` | Permanently Open |
| `10` | Lock (read/write requires access password) |
| `11` | Permanently Lock |

> Only Action bits whose corresponding Mask bit is `1` are applied.  
> Reference: EPC Gen2 v1.2.0, §6.3.2.11.3.5.

**Example** — Lock Access Password:

```
BB 00 82 00 07 00 00 FF FF 02 00 80 09 7E
```

#### 2.10.2 Response Frame (success)

```
BB 01 82 00 10 0E 34 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 E2 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| Tag not found / EPC mismatch | `0x13` |
| Incorrect Access Password | `0x16` (+ PC + EPC) |
| EPC Gen2 tag error | `0xC0 \| error_code` |

---

### 2.11 Kill Tag

Permanently disables a tag. Precede with Select parameter setup.

> **Warning:** A tag with Kill Password = `0x00000000` cannot be killed per EPC Gen2 spec.

#### 2.11.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x65`  
- **Parameter Length:** `0x0004`

```
BB 00 65 00 04 00 00 FF FF 67 7E
```

#### 2.11.2 Response Frame (success)

```
BB 01 65 00 10 0E 34 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 C5 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| Tag not found / EPC mismatch | `0x12` |
| Kill Password = 0 (tag not killed) | `0xD0` (+ PC + EPC) |
| EPC Gen2 tag error | `0xD0 \| error_code` |

---

### 2.12 Set Communication Baud Rate

Changes the UART baud rate. After execution the host must reconnect at the new rate.  
**No Notification Frame** is returned for this command.

#### 2.12.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x11`  
- **Parameter Length:** `0x0002`  
- **Pow:** `baud_rate / 100` as a 16-bit big-endian value

**Example** — Set 19200 baud (`19200 / 100 = 192 = 0x00C0`):

```
BB 00 11 00 02 00 C0 D3 7E
```

---

### 2.13 Get Query Parameters

Retrieves the current EPC Gen2 Query command parameters.

#### 2.13.1 Command Frame

```
BB 00 0D 00 00 0D 7E
```

#### 2.13.2 Response Frame

- **Parameter:** 2 bytes, bit-concatenation of:

| Field | Width | Encoding |
|-------|-------|----------|
| DR | 1 bit | `0`=8, `1`=64/3 (only DR=8 supported) |
| M | 2 bits | `00`=1, `01`=2, `10`=4, `11`=8 (only M=1 supported) |
| TRext | 1 bit | `0`=no pilot tone, `1`=pilot tone (only `1` supported) |
| Sel | 2 bits | `00`/`01`=ALL, `10`=~SL, `11`=SL |
| Session | 2 bits | `00`=S0, `01`=S1, `10`=S2, `11`=S3 |
| Target | 1 bit | `0`=A, `1`=B |
| Q | 4 bits | `0000`–`1111` |

**Example response** — DR=8, M=1, TRext=pilot, Sel=ALL, Session=S0, Target=A, Q=4 → `0x1020`:

```
BB 01 0D 00 02 10 20 40 7E
```

---

### 2.14 Set Query Parameters

Sets the EPC Gen2 Query parameters. Same bit layout as §2.13.

#### 2.14.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x0E`  
- **Parameter Length:** `0x0002`

**Example** — DR=8, M=1, TRext=pilot, Sel=ALL, Session=S0, Target=A, Q=4:

```
BB 00 0E 00 02 10 20 40 7E
```

#### 2.14.2 Response Frame (success)

```
BB 01 0E 00 01 00 10 7E
```

---

### 2.15 Set Operating Region

Configures the RF regulatory region.

#### 2.15.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x07`  
- **Parameter Length:** `0x0001`

| Region | Code |
|--------|------|
| China 900 MHz | `0x01` |
| China 800 MHz | `0x04` |
| America | `0x02` |
| Europe | `0x03` |
| South Korea | `0x06` |

**Example** — China 900 MHz:

```
BB 00 07 00 01 01 09 7E
```

#### 2.15.2 Response Frame (success)

```
BB 01 07 00 01 00 09 7E
```

---

### 2.16 Get Operating Region

Returns the current regulatory region code.

#### 2.16.1 Command Frame

```
BB 00 08 00 00 08 7E
```

#### 2.16.2 Response Frame

- **Command:** `0x08`, **Parameter:** region code byte

```
BB 01 08 00 01 01 0B 7E
```

Region code table same as §2.15.

---

### 2.17 Set Operating Channel

Sets a fixed operating channel within the current region.

#### 2.17.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xAB`  
- **Parameter Length:** `0x0001`  
- **Parameter:** `CH_Index` (byte)

**CH_Index formulas:**

| Region | Formula |
|--------|---------|
| China 900 MHz | `CH_Index = (Freq_MHz − 920.125) / 0.25` |
| China 800 MHz | `CH_Index = (Freq_MHz − 840.125) / 0.25` |
| America | `CH_Index = (Freq_MHz − 902.25) / 0.5` |
| Europe | `CH_Index = (Freq_MHz − 865.1) / 0.2` |
| South Korea | `CH_Index = (Freq_MHz − 917.1) / 0.2` |

**Example** — 920.375 MHz (China 900 → CH_Index = 1):

```
BB 00 AB 00 01 01 AD 7E
```

#### 2.17.2 Response Frame (success)

```
BB 01 AB 00 01 00 AD 7E
```

---

### 2.18 Get Operating Channel

Returns the current channel index.

#### 2.18.1 Command Frame

```
BB 00 AA 00 00 AA 7E
```

#### 2.18.2 Response Frame

```
BB 01 AA 00 01 00 AC 7E
```

**CH_Index → Frequency formulas:**

| Region | Formula |
|--------|---------|
| China 900 MHz | `Freq_MHz = CH_Index × 0.25 + 920.125` |
| China 800 MHz | `Freq_MHz = CH_Index × 0.25 + 840.125` |
| America | `Freq_MHz = CH_Index × 0.5 + 902.25` |
| Europe | `Freq_MHz = CH_Index × 0.2 + 865.1` |
| South Korea | `Freq_MHz = CH_Index × 0.2 + 917.1` |

---

### 2.19 Set Automatic Frequency Hopping

Enables or disables automatic frequency hopping.

When enabled, the reader hops among user-defined channels (if set via §2.20) or its internal preset channel list.

#### 2.19.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xAD`  
- **Parameter:** `0xFF` = enable hopping, `0x00` = disable

```
BB 00 AD 00 01 FF AD 7E
```

#### 2.19.2 Response Frame (success)

```
BB 01 AD 00 01 00 AF 7E
```

---

### 2.20 Insert Operating Channel

Defines a custom channel list for frequency hopping. After this command, the reader hops only among these channels.

#### 2.20.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xA9`  
- **Parameter Length:** `0x0001 + CH_Cnt`
- Fields: `CH_Cnt (1 byte)` + `CH_List (CH_Cnt bytes)`

**Example** — 5 channels (indices 1–5):

```
BB 00 A9 00 06 05 01 02 03 04 05 C3 7E
```

#### 2.20.2 Response Frame (success)

```
BB 01 A9 00 01 00 AB 7E
```

---

### 2.21 Get Transmit Power

Returns the current output power.

#### 2.21.1 Command Frame

```
BB 00 B7 00 00 B7 7E
```

#### 2.21.2 Response Frame

- **Parameter:** 2-byte value = `power_dBm × 100`

**Example** — 20 dBm (`2000 = 0x07D0`):

```
BB 01 B7 00 02 07 D0 91 7E
```

---

### 2.22 Set Transmit Power

Sets the output power.

#### 2.22.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xB6`  
- **Parameter Length:** `0x0002`  
- **Pow:** `power_dBm × 100` as big-endian 16-bit

**Example** — 20 dBm:

```
BB 00 B6 00 02 07 D0 8F 7E
```

#### 2.22.2 Response Frame (success)

```
BB 01 B6 00 01 00 B8 7E
```

---

### 2.23 Set Continuous Carrier Transmission

Turns the CW (continuous wave) carrier on or off.

#### 2.23.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xB0`  
- **Parameter:** `0xFF` = on, `0x00` = off

```
BB 00 B0 00 01 FF B0 7E
```

#### 2.23.2 Response Frame (success)

```
BB 01 B0 00 01 00 B2 7E
```

---

### 2.24 Get Receiver Demodulator Parameters

Retrieves Mixer gain, IF amplifier gain, and signal demodulation threshold.

#### 2.24.1 Command Frame

```
BB 00 F1 00 00 F1 7E
```

#### 2.24.2 Response Frame

- **Fields:** `Mixer_G (1)` + `IF_G (1)` + `Thrd (2)`

```
BB 01 F1 00 04 03 06 01 B0 B0 7E
```

**Mixer Gain table:**

| Mixer_G | dB |
|---------|----|
| `0x00` | 0 |
| `0x01` | 3 |
| `0x02` | 6 |
| `0x03` | 9 |
| `0x04` | 12 |
| `0x05` | 15 |
| `0x06` | 16 |

**IF AMP Gain table:**

| IF_G | dB |
|------|----|
| `0x00` | 12 |
| `0x01` | 18 |
| `0x02` | 21 |
| `0x03` | 24 |
| `0x04` | 27 |
| `0x05` | 30 |
| `0x06` | 36 |
| `0x07` | 40 |

> **Thrd:** Lower value = more sensitive but less stable; `0x01B0` is the recommended minimum.

---

### 2.25 Set Receiver Demodulator Parameters

Configures Mixer gain, IF amplifier gain, and demodulation threshold.

#### 2.25.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xF0`  
- **Parameter Length:** `0x0004`

Same field order as §2.24.2 response.

```
BB 00 F0 00 04 03 06 01 B0 AE 7E
```

Gain tables same as §2.24.

#### 2.25.2 Response Frame (success)

```
BB 01 F0 00 01 00 F2 7E
```

---

### 2.26 Test RF Input Blocking Signal

Scans all channels in the current region and measures the jammer (blocking) signal level.

#### 2.26.1 Command Frame

```
BB 00 F2 00 00 F2 7E
```

#### 2.26.2 Response Frame

- **Fields:** `CH_L (1)` + `CH_H (1)` + `JMR[CH_L..CH_H]` (one signed byte per channel, unit dBm)

**Example** — China 900 MHz, 20 channels (index 0–19):

```
BB 01 F2 00 16 00 13
F2 F1 F0 EF EC EA E8 EA EC EE F0 F1 F5 F5 F5 F6 F5 F5 F5 F5
DD 7E
```

> `0xF2` as signed byte = −14 dBm.

---

### 2.27 Test Channel RSSI

Measures the ambient RF energy (RSSI) on each channel to detect nearby readers.

#### 2.27.1 Command Frame

```
BB 00 F3 00 00 F3 7E
```

#### 2.27.2 Response Frame

Same structure as §2.26.2 but field is RSSI per channel (signed byte, unit dBm).

```
BB 01 F3 00 16 00 13
BA BA BA BA BA BA BA BA BA BA BA BA BA BA BA BA BA BA BA BA
A5 7E
```

---

### 2.28 Control IO Port

Sets IO direction, reads IO level, or writes IO level for ports IO1–IO4.

#### 2.28.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x1A`  
- **Parameter Length:** `0x0003`  
- **Fields:** `Parameter0` + `Parameter1` + `Parameter2`

| Byte | Field | Values |
|------|-------|--------|
| 0 | Operation type | `0x00`=Set direction, `0x01`=Set level, `0x02`=Read level |
| 1 | Target port | `0x01`–`0x04` (IO1–IO4) |
| 2 | Value | See table below |

**Parameter2 meanings:**

| Op (P0) | P2 | Result |
|---------|----|--------|
| `0x00` (set dir) | `0x00` | Input mode |
| `0x00` (set dir) | `0x01` | Output mode |
| `0x01` (set level) | `0x00` | Output low |
| `0x01` (set level) | `0x01` | Output high |
| `0x02` (read) | — | Ignored |

**Example** — Set IO4 as output mode:

```
BB 01 1A 00 03 00 04 01 22 7E
```

#### 2.28.2 Response Frame

Same structure. Parameter2 in the response indicates result:

| Op (P0) | P2 | Result |
|---------|----|--------|
| `0x00` | `0x00` | IO configuration failed |
| `0x00` | `0x01` | IO configuration succeeded |
| `0x01` | `0x00` | Set IO level failed |
| `0x01` | `0x01` | Set IO level succeeded |
| `0x02` | `0x00` | Port is at low level |
| `0x02` | `0x01` | Port is at high level |

---

### 2.29 Module Sleep

Places the module in low-power sleep. Any serial byte wakes it, but that byte is discarded; the first command after waking also receives no response (its first byte was consumed).

**On wake:** M100/QM100 is power-down reset, firmware re-downloads, and these settings are restored: power, frequency, frequency-hopping mode, sleep duration, demodulator parameters. **Excluded:** Select mode and Select parameters.

#### 2.29.1 Command Frame

```
BB 00 17 00 00 17 7E
```

#### 2.29.2 Response Frame (success)

```
BB 01 17 00 01 00 19 7E
```

---

### 2.30 Module Idle Sleep Time

Sets the inactivity timeout before the module auto-enters sleep. Same wake/restore behavior as §2.29.

#### 2.30.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x1D`  
- **Parameter:** timeout in minutes (`0x00` = never auto-sleep; range 1–30)

**Example** — Auto-sleep after 2 minutes:

```
BB 00 1D 00 01 02 20 7E
```

#### 2.30.2 Response Frame (success)

```
BB 01 1D 00 01 02 21 7E
```

---

### 2.31 IDLE Mode

Turns off all analog and RF power supplies (except digital section and UART). Communication remains functional; all parameters are retained. The first inventory/tag-interaction after entering IDLE may have a lower success rate while RF circuitry stabilizes.

#### 2.31.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0x04`  
- **Parameter Length:** `0x0003`

| Field | Value | Description |
|-------|-------|-------------|
| Enter | `0x01` | `0x01`=enter IDLE, `0x00`=exit IDLE |
| Reserved | `0x01` | Fixed |
| IDLE Time | byte | Auto-IDLE timeout in minutes (`0x00`=disable; range 0–30) |

**Example** — Enter IDLE, auto-IDLE after 3 min:

```
BB 01 04 00 03 01 01 03 0C 7E
```

#### 2.31.2 Response Frame (success)

```
BB 01 04 00 01 00 06 7E
```

---

### 2.32 NXP ReadProtect / Reset ReadProtect

For **NXP G2X** tags. ReadProtect sets `ProtectEPC` and `ProtectTID` to `1` (data-protected state). Reset ReadProtect restores normal operation.

#### 2.32.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xE1`  
- **Parameter Length:** `0x0005`

| Field | Size | Value |
|-------|------|-------|
| Access Password | 4 bytes | |
| Set/Reset | 1 byte | `0x00`=ReadProtect, `0x01`=Reset ReadProtect |

```
BB 00 E1 00 05 00 00 FF FF 00 E4 7E
```

#### 2.32.2 Response Frame

**ReadProtect success** (Command=`0xE1`):

```
BB 01 E1 00 10 0E 30 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 3D 7E
```

**Reset ReadProtect success** (Command=`0xE2`):

```
BB 01 E2 00 10 0E 30 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 3E 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| ReadProtect failed (no tag / bad EPC) | `0x2A` |
| Reset ReadProtect failed (no tag / bad EPC) | `0x2B` |
| Incorrect Access Password | `0x16` (+ PC + EPC) |

---

### 2.33 NXP Change EAS Command

For **NXP G2X** tags. Sets or clears the `PSF` bit. When PSF=1 the tag responds to EAS_Alarm; when PSF=0 it does not.

#### 2.33.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xE3`  
- **Parameter Length:** `0x0005`

| Field | Size | Description |
|-------|------|-------------|
| Access Password | 4 bytes | |
| PSF bit | 1 byte | `0x01`=set PSF=1, `0x00`=clear PSF=0 |

```
BB 00 E3 00 05 00 00 FF FF 01 E7 7E
```

#### 2.33.2 Response Frame (success)

```
BB 01 E3 00 10 0E 30 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 3F 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| Tag not found / EPC mismatch | `0x1B` |
| Incorrect Access Password | `0x16` (+ PC + EPC) |

---

### 2.34 NXP EAS_Alarm Command

Broadcasts EAS_Alarm; tags with PSF=1 respond with a 64-bit Alarm code. Useful for electronic article surveillance (EAS) systems.

#### 2.34.1 Command Frame

```
BB 00 E4 00 00 E4 7E
```

#### 2.34.2 Response Frame (success)

- **Parameter Length:** `0x0008` (8-byte EAS-Alarm code)

```
BB 01 E4 00 08 69 0A EC 7C D2 15 D8 F9 80 7E
```

#### Error Response

| Error | Code |
|-------|------|
| No tag responded | `0x1D` — `BB 01 FF 00 01 1D 1E 7E` |

---

### 2.35 NXP ChangeConfig Command

For **NXP G2X** tags (e.g., G2iM, G2iM+). Reads or modifies the 16-bit Config-Word located at EPC bank address `0x20h`. Writing flips targeted bits: writing `1` inverts the bit; writing `0` leaves it unchanged.

#### 2.35.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xE0`  
- **Parameter Length:** `0x0006`

| Field | Size | Description |
|-------|------|-------------|
| Access Password | 4 bytes | |
| Config-Word | 2 bytes | `0x0000` = read (returns unchanged Config-Word) |

```
BB 00 E0 00 06 00 00 FF FF 00 00 E4 7E
```

#### 2.35.2 Response Frame (success)

- **Fields:** `UL` + `PC` + `EPC` + `Config-Word (2 bytes)`

```
BB 01 E0 00 11 0E 30 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 41 7E 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| Tag not found / EPC mismatch | `0x1A` |
| Incorrect Access Password | `0x16` (+ PC + EPC) |

---

### 2.36 Impinj Monza QT Command

For **Impinj Monza 4 QT** tags. Modifies the QT Control word.

| Bit | Field | Description |
|-----|-------|-------------|
| MSB | QT_SR | Shorten operating range in Open/Secured state |
| MSB-1 | QT_MEM | Switch between Public and Private memory map |

#### 2.36.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xE5`  
- **Parameter Length:** `0x0008`

| Field | Size | Description |
|-------|------|-------------|
| Access Password | 4 bytes | |
| Read/Write | 1 byte | `0x00`=Read, `0x01`=Write |
| Persistence | 1 byte | `0x00`=volatile, `0x01`=non-volatile |
| Payload | 2 bytes | QT Control word |

**Example** — Write QT_MEM=1 to non-volatile memory (`Payload=0x4000`):

```
BB 00 E5 00 08 00 00 FF FF 01 01 40 00 2D 7E
```

#### 2.36.2 Response Frame

**Read (R/W=0x00)** — returns QT Control word (Command=`0xE5`):

```
BB 01 E5 00 11 0E 30 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 00 42 7E
```

**Write (R/W=0x01)** — returns success status (Command=`0xE6`):

```
BB 01 E6 00 10 0E 30 00 30 75 1F EB 70 5C 59 04 E3 D5 0D 70 00 42 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| Tag not found / EPC mismatch | `0x2E` |
| Incorrect Access Password | `0x16` (+ PC + EPC) |

---

### 2.37 BlockPermalock Command

Permanently locks specific user-memory blocks, or reads their lock status. Precede with Select parameter setup.

#### 2.37.1 Command Frame

- **Type:** `0x00`  
- **Command:** `0xD3`  
- **Parameter Length:** `0x000B`

| Field | Size | Description |
|-------|------|-------------|
| Access Password | 4 bytes | |
| Read/Lock | 1 byte | `0x00`=Read status, `0x01`=Lock |
| MemBank | 1 byte | Memory bank |
| BlockPtr | 2 bytes | Start block address of mask (units: 16 blocks) |
| BlockRange | 1 byte | Number of 16-block units |
| Mask | 2 bytes | Lock mask (omitted when Read/Lock=0x00) |

**Example** — Permanently lock blocks 5, 6, 7 (BlockPtr=0, Range=1, Mask=`0x0700`):

```
BB 00 D3 00 0B 00 00 FF FF 01 03 00 00 01 07 00 E8 7E
```

#### 2.37.2 Response Frame

**Read (R/L=0x00)** — returns lock status (Command=`0xD3`):

- **Fields:** `UL` + `PC` + `EPC` + `BlockRange (1)` + `Data (2)`

```
BB 01 D3 00 12 0E 30 00 E2 00 30 16 66 06 00 69 11 60 9F 70 01 07 00 CD 7E
```

**Lock (R/L=0x01)** — returns success status (Command=`0xD4`):

```
BB 01 D4 00 10 0E 30 00 E2 00 30 16 66 06 00 69 11 60 9F 94 00 C4 7E
```

#### Error Responses

| Error | Code |
|-------|------|
| Tag not found / EPC mismatch | `0x14` |
| EPC Gen2 tag error | `0xE0 \| error_code` (e.g., Memory Overrun → `0xE3`) |
| Incorrect Access Password | `0x16` (+ PC + EPC) |

---

## 3. Command Summary

| Command Code | Name |
|-------------|------|
| `0x03` | Get Reader Module Information |
| `0x22` | Single Polling Command |
| `0x27` | Multiple Polling Command |
| `0x28` | Stop Multiple Polling Command |
| `0x0C` | Set Select Parameter Command |
| `0x0B` | Get Select Parameter |
| `0x12` | Set Select Mode |
| `0x39` | Read Tag Memory Area |
| `0x49` | Write Tag Memory Area |
| `0x82` | Lock Tag Memory Area |
| `0x65` | Kill Tag |
| `0x0D` | Get Query Parameters |
| `0x0E` | Set Query Parameters |
| `0x07` | Set Operating Region |
| `0x08` | Get Operating Region |
| `0xAB` | Set Operating Channel |
| `0xAA` | Get Operating Channel |
| `0xAD` | Set Automatic Frequency Hopping |
| `0xA9` | Insert Operating Channel |
| `0xB7` | Get Transmit Power |
| `0xB6` | Set Transmit Power |
| `0xB0` | Set Continuous Carrier Transmission |
| `0xF1` | Get Receiver Demodulator Parameters |
| `0xF0` | Set Receiver Demodulator Parameters |
| `0xF2` | Test RF Input Blocking Signal |
| `0xF3` | Test Channel RSSI |
| `0x1A` | Control IO Port |
| `0x11` | Set Communication Baud Rate |
| `0x17` | Module Sleep |
| `0x1D` | Set Module Idle Sleep Time |
| `0x04` | IDLE Mode |
| `0xE0` | NXP ChangeConfig Command |
| `0xE1` | NXP ReadProtect / Reset ReadProtect |
| `0xE3` | NXP Change EAS Command |
| `0xE4` | NXP EAS_Alarm Command |
| `0xE5` / `0xE6` | Impinj Monza 4 QT Command |
| `0xD3` / `0xD4` | BlockPermalock Command |

---

## 4. Error Code Summary

When a Command Frame fails the M100 responds with Command=`0xFF`. If the tag EPC was not yet retrieved, the Parameter field is a 1-byte error code only. If the EPC was retrieved, the frame contains the error code followed by the tag's PC + EPC.

### General Error Codes

| Type | Code | Description |
|------|------|-------------|
| Command Error | `0x17` | Malformed Command Frame |
| FHSS Fail | `0x20` | Frequency-hopping channel search timed out; all channels occupied |
| Inventory Fail | `0x15` | Polling failed — no tag response or Data CRC error |
| Access Fail | `0x16` | Tag access failed — likely incorrect access password |
| Read Fail | `0x09` | Tag memory read failed — no response or CRC error |
| Read Error | `0xA0 \| err` | EPC Gen2 error during read; OR error code with `0xA0` |
| Write Fail | `0x10` | Tag memory write failed — no response or CRC error |
| Write Error | `0xB0 \| err` | EPC Gen2 error during write; OR error code with `0xB0` |
| Lock Fail | `0x13` | Tag lock failed — no response or CRC error |
| Lock Error | `0xC0 \| err` | EPC Gen2 error during lock; OR error code with `0xC0` |
| Kill Fail | `0x12` | Tag kill failed — no response or CRC error |
| Kill Error | `0xD0 \| err` | EPC Gen2 error during kill; OR error code with `0xD0` |
| BlockPermalock Fail | `0x14` | No response or CRC error |
| BlockPermalock Error | `0xE0 \| err` | EPC Gen2 error; OR error code with `0xE0` |

### NXP G2X Tag-Specific Error Codes

| Type | Code | Description |
|------|------|-------------|
| ChangeConfig Fail | `0x1A` | No data returned or CRC error |
| ReadProtect Fail | `0x2A` | No data returned or CRC error |
| Reset ReadProtect Fail | `0x2B` | No data returned or CRC error |
| Change EAS Fail | `0x1B` | No data returned or CRC error |
| EAS_Alarm Fail | `0x1D` | No tag returned Alarm code |
| Tag proprietary error | `0xE0 \| err` | OR tag error code with `0xE0` |

### Impinj Monza QT Tag-Specific Error Codes

| Type | Code | Description |
|------|------|-------------|
| QT Fail | `0x2E` | No data returned or CRC error |
| Tag proprietary error | `0xE0 \| err` | OR tag error code with `0xE0` |

### EPC Gen2 Protocol Tag Error Codes

| Error Code | Name | Description |
|-----------|------|-------------|
| `0b00000000` | Other error | Unspecified error |
| `0b00000011` | Memory overrun | Specified memory does not exist, or unsupported EPC length |
| `0b00000100` | Memory locked | Memory area is locked; non-writable or non-readable |
| `0b00001011` | Insufficient power | Tag did not receive enough power for the write operation |
| `0b00001111` | Non-specific error | Tag does not support returning an error code |

> **Note:** Only the lower 4 bits of EPC Gen2 error codes are valid. The M100 ORs the base prefix with the raw error code before returning it.

---

*End of document.*
