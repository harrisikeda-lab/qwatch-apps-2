# Qwatch BLE Protocol Specification

**Status:** Phase 2 Analysis  
**Date:** 2026-06-11  
**Device:** ESP32-S3 with Bluetooth LE  
**Target Companion:** Flutter (iOS + Android)

---

## 1. Bluetooth Architecture

### 1.1 Current State
- ✅ **Bluetooth Controller:** Initialized (btStart/btStop)
- ✅ **Capability:** Enabled/disabled via `time_sync_set_bluetooth_active(bool)`
- ❌ **GATT Server:** NOT implemented
- ❌ **Services:** NOT defined
- ❌ **Characteristics:** NOT defined
- ❌ **UUIDs:** NOT assigned

### 1.2 Device Profile
```c
Device Name:           "Qwatch"
Device Role:           Peripheral (GATT Server)
Advertising Interval:  100ms (standard BLE)
Connection Interval:   15-100ms (negotiable)
MTU Size:             23 bytes (default) → can negotiate to 517 bytes
PHY:                  1M (standard BLE)
```

### 1.3 Advertising Packet
**Required for Phase 2:**
```
Flags:                  0x06 (LE General Discoverable, BR/EDR Not Supported)
Complete Local Name:    "Qwatch"
Advertised Services:    [Primary Service UUIDs - 6 services]
Manufacturer Data:      (Optional) Version, battery, etc.
```

---

## 2. BLE Service Architecture

### 2.1 Service Hierarchy

```
Qwatch GATT Server
├── Service 1: Device Information (0x180A - Standard)
│   ├── Char: Manufacturer Name (R)
│   ├── Char: Model Number (R)
│   ├── Char: Firmware Revision (R)
│   └── Char: Serial Number (R)
│
├── Service 2: Battery (0x180F - Standard)
│   ├── Char: Battery Level (R, N)
│   └── Char: Battery Status (R, N) [Custom extension]
│
├── Service 3: Device Status (Custom UUID)
│   ├── Char: Current Time (R, N)
│   ├── Char: Timezone Offset (R, N)
│   ├── Char: WiFi SSID (R)
│   ├── Char: WiFi Status (R, N)
│   └── Char: Bluetooth Active (R, W, N)
│
├── Service 4: Settings Control (Custom UUID)
│   ├── Char: Clock Mode (R, W, N)
│   ├── Char: Digital Face Index (R, W)
│   ├── Char: Analogue Face Index (R, W)
│   ├── Char: WiFi Credentials (W) [Write-only, encrypted]
│   └── Char: Settings Sync Request (W, I)
│
├── Service 5: Pedometer (Custom UUID)
│   ├── Char: Current Steps (R, N)
│   ├── Char: Daily History (R) [Large read, may use long CCCD]
│   ├── Char: Reset Steps (W, I) [Write & Indicate complete]
│   └── Char: Pedometer Active (R, W)
│
└── Service 6: Weather & Location (Custom UUID)
    ├── Char: Temperature (R, N)
    ├── Char: Weather Icon Code (R, N)
    ├── Char: Location City (R)
    ├── Char: Latitude (R)
    ├── Char: Longitude (R)
    └── Char: Weather Update Trigger (W)
```

---

## 3. Service Definitions

### 3.1 Service 1: Device Information (Standard 0x180A)

| Characteristic | UUID | Type | Permissions | Format | Description |
|---|---|---|---|---|---|
| Manufacturer Name | 0x2A29 | String | Read | UTF-8 | "Harris Ikeda" |
| Model Number | 0x2A24 | String | Read | UTF-8 | "ESP32-S3-Touch-LCD-1.28" |
| Firmware Revision | 0x2A26 | String | Read | UTF-8 | "v5.7" (from QWATCH_VERSION) |
| Serial Number | 0x2A25 | String | Read | UTF-8 | MAC address or unique ID |

**Operations:**
- ✅ **Read:** Always available
- ❌ **Write:** Not supported
- ❌ **Notify:** Not supported

---

### 3.2 Service 2: Battery (Standard 0x180F + Custom)

#### Characteristic 2A: Battery Level (Standard 0x2A19)
```
UUID:           0x2A19
Type:           Unsigned 8-bit integer
Permissions:    Read, Notify
Range:          0-100 (%)
Update Rate:    Every 5 seconds (on change)
```

**Notification Format:**
```c
uint8_t battery_percent;  // 0-100
```

#### Characteristic 2B: Battery Status (Custom)
```
UUID:           To be assigned (e.g., 0xBB01)
Type:           Struct
Permissions:    Read, Notify
Update Rate:    Every 5 seconds
```

**Read/Notify Format:**
```json
{
  "percent": 85,
  "voltage_mv": 3950,
  "charging": false,
  "health": "good"
}
```

**Binary Format (7 bytes):**
```
Byte 0:      Battery percent (0-100)
Bytes 1-2:   Voltage in mV (16-bit LE)
Byte 3:      Charging status (0=no, 1=yes)
Byte 4:      Health (0=good, 1=warning, 2=critical)
Bytes 5-6:   Reserved
```

---

### 3.3 Service 3: Device Status (Custom UUID: 0xAA00)

#### Characteristic 3A: Current Time (UUID: 0xAA01)
```
UUID:           0xAA01
Type:           Struct (8 bytes)
Permissions:    Read, Notify
Update Rate:    Every 1 second
```

**Format (8 bytes):**
```c
struct {
  uint32_t unix_timestamp;  // Seconds since epoch
  uint16_t year;            // 2026
  uint8_t month;            // 1-12
  uint8_t day;              // 1-31
}
```

#### Characteristic 3B: Timezone Offset (UUID: 0xAA02)
```
UUID:           0xAA02
Type:           Signed 32-bit integer
Permissions:    Read, Notify
Unit:           Seconds offset from UTC
Example:        -18000 (UTC-5 EST)
Update Rate:    On weather sync (hourly)
```

#### Characteristic 3C: WiFi SSID (UUID: 0xAA03)
```
UUID:           0xAA03
Type:           String
Permissions:    Read
Max Length:     32 bytes
Format:         UTF-8, null-terminated
```

#### Characteristic 3D: WiFi Status (UUID: 0xAA04)
```
UUID:           0xAA04
Type:           Struct (3 bytes)
Permissions:    Read, Notify
Update Rate:    On connection change, then every 30 seconds
```

**Format (3 bytes):**
```c
struct {
  uint8_t status;    // 0=off, 1=connecting, 2=connected, 3=error
  int8_t rssi;       // dBm (-120 to 0)
  uint8_t reserved;
}
```

#### Characteristic 3E: Bluetooth Active (UUID: 0xAA05)
```
UUID:           0xAA05
Type:           Boolean (1 byte)
Permissions:    Read, Write, Notify
Values:         0=off, 1=on
Update Rate:    On write, then every 5 seconds
```

**Operations:**
- **Read:** Returns current Bluetooth state
- **Write:** Toggle Bluetooth (calls `time_sync_set_bluetooth_active()`)
- **Notify:** On state change

---

### 3.4 Service 4: Settings Control (Custom UUID: 0xBB00)

#### Characteristic 4A: Clock Mode (UUID: 0xBB01)
```
UUID:           0xBB01
Type:           Enumeration (1 byte)
Permissions:    Read, Write, Notify
Values:         0=Digital, 1=Analogue
Update Rate:    On write
```

**Operations:**
- **Read:** Returns current mode from `qwatch_settings_get_clock_mode()`
- **Write:** Calls `qwatch_settings_set_clock_mode()`
- **Notify:** On write from companion app or watch UI

#### Characteristic 4B: Digital Face Index (UUID: 0xBB02)
```
UUID:           0xBB02
Type:           Unsigned 8-bit integer
Permissions:    Read, Write
Range:          0-N (N = number of digital faces)
Update Rate:    On write
```

**Operations:**
- **Read:** Returns from `qwatch_settings_get_digital_face_index()`
- **Write:** Calls `qwatch_settings_set_digital_face_index(value)`

#### Characteristic 4C: Analogue Face Index (UUID: 0xBB03)
```
UUID:           0xBB03
Type:           Unsigned 8-bit integer
Permissions:    Read, Write
Range:          0-N (N = number of analogue faces)
Update Rate:    On write
```

#### Characteristic 4D: WiFi Credentials (UUID: 0xBB04)
```
UUID:           0xBB04
Type:           Encrypted Binary
Permissions:    Write Only
Max Length:     256 bytes
Format:         AES-128-GCM encrypted JSON
```

**Plaintext JSON (before encryption):**
```json
{
  "ssid": "NetworkName",
  "password": "SecurePassword123",
  "timestamp": 1686000000
}
```

**Encryption:**
- **Algorithm:** AES-128-GCM
- **Key:** Derived from watch MAC address (PBKDF2)
- **IV:** Random 12 bytes, prepended to ciphertext
- **Auth Tag:** Last 16 bytes

**Operations:**
- **Write:** Companion app sends encrypted credentials
- **Read:** Not permitted (security)
- **Notify:** Not applicable

#### Characteristic 4E: Settings Sync Request (UUID: 0xBB05)
```
UUID:           0xBB05
Type:           Command (1 byte)
Permissions:    Write, Indicate
Values:         0x01=request sync, 0x02=sync complete
```

**Operations:**
- **Write:** App sends 0x01 to request all settings dump
- **Indicate:** Watch sends 0x02 when sync complete (with confirmation)

---

### 3.5 Service 5: Pedometer (Custom UUID: 0xCC00)

#### Characteristic 5A: Current Steps (UUID: 0xCC01)
```
UUID:           0xCC01
Type:           Unsigned 32-bit integer (4 bytes)
Permissions:    Read, Notify
Unit:           Steps
Update Rate:    Every 10 seconds or on step change
```

**Format (4 bytes, little-endian):**
```c
uint32_t step_count;
```

#### Characteristic 5B: Daily History (UUID: 0xCC02)
```
UUID:           0xCC02
Type:           Binary (variable length)
Permissions:    Read
Max Length:     256 bytes (7 days × ~30 bytes)
Format:         JSON array
```

**Read Response Format:**
```json
[
  {
    "date": "2026-06-11",
    "steps": 8432,
    "calories": 312
  },
  {
    "date": "2026-06-10",
    "steps": 6521,
    "calories": 245
  }
]
```

**Operations:**
- **Read:** Returns last 7 days of data
- **Indicate:** Not used (read is on-demand)

#### Characteristic 5C: Reset Steps (UUID: 0xCC03)
```
UUID:           0xCC03
Type:           Command (1 byte)
Permissions:    Write, Indicate
Values:         0x01=reset
```

**Operations:**
- **Write:** App sends 0x01 to reset step counter
- **Indicate:** Watch responds with 0x00 (success) or 0x01 (error)

#### Characteristic 5D: Pedometer Active (UUID: 0xCC04)
```
UUID:           0xCC04
Type:           Boolean (1 byte)
Permissions:    Read, Write, Notify
Values:         0=paused, 1=active
Update Rate:    On write or pause event
```

---

### 3.6 Service 6: Weather & Location (Custom UUID: 0xDD00)

#### Characteristic 6A: Temperature (UUID: 0xDD01)
```
UUID:           0xDD01
Type:           Signed 8-bit integer
Permissions:    Read, Notify
Unit:           °C (-40 to 60)
Update Rate:    Hourly or on refresh
```

#### Characteristic 6B: Weather Icon Code (UUID: 0xDD02)
```
UUID:           0xDD02
Type:           Unsigned 16-bit integer
Permissions:    Read, Notify
Unit:           WMO weather code (0-99)
Mapping:        See weather_service.cpp map_weather_icon()
Update Rate:    Hourly or on refresh
```

#### Characteristic 6C: Location City (UUID: 0xDD03)
```
UUID:           0xDD03
Type:           String
Permissions:    Read
Max Length:     64 bytes
Format:         UTF-8, null-terminated
Example:        "San Francisco"
```

#### Characteristic 6D: Latitude (UUID: 0xDD04)
```
UUID:           0xDD04
Type:           Floating-point (4 bytes, IEEE 754)
Permissions:    Read
Unit:           Decimal degrees
Range:          -90 to +90
```

#### Characteristic 6E: Longitude (UUID: 0xDD05)
```
UUID:           0xDD05
Type:           Floating-point (4 bytes, IEEE 754)
Permissions:    Read
Unit:           Decimal degrees
Range:          -180 to +180
```

#### Characteristic 6F: Weather Update Trigger (UUID: 0xDD06)
```
UUID:           0xDD06
Type:           Command (1 byte)
Permissions:    Write, Indicate
Values:         0x01=refresh now
```

**Operations:**
- **Write:** App sends 0x01 to trigger immediate weather refresh
- **Indicate:** Watch responds with 0x00 (success) or 0x01 (no WiFi)

---

## 4. UUID Definitions

### Standard BLE UUIDs (128-bit Base: 0000xxxx-0000-1000-8000-00805F9B34FB)

| Service/Characteristic | Standard UUID | Custom UUID | Type |
|---|---|---|---|
| Device Information | 0x180A | - | Standard |
| Manufacturer Name | 0x2A29 | - | Standard |
| Model Number | 0x2A24 | - | Standard |
| Firmware Revision | 0x2A26 | - | Standard |
| Serial Number | 0x2A25 | - | Standard |
| Battery Service | 0x180F | - | Standard |
| Battery Level | 0x2A19 | - | Standard |
| Battery Status | - | 0x000000BB-xxxx... | Custom |
| **Device Status Service** | - | 0x0000AA00-... | Custom |
| Current Time | - | 0x0000AA01-... | Custom |
| Timezone Offset | - | 0x0000AA02-... | Custom |
| WiFi SSID | - | 0x0000AA03-... | Custom |
| WiFi Status | - | 0x0000AA04-... | Custom |
| Bluetooth Active | - | 0x0000AA05-... | Custom |
| **Settings Service** | - | 0x0000BB00-... | Custom |
| Clock Mode | - | 0x0000BB01-... | Custom |
| Digital Face Index | - | 0x0000BB02-... | Custom |
| Analogue Face Index | - | 0x0000BB03-... | Custom |
| WiFi Credentials | - | 0x0000BB04-... | Custom |
| Settings Sync Request | - | 0x0000BB05-... | Custom |
| **Pedometer Service** | - | 0x0000CC00-... | Custom |
| Current Steps | - | 0x0000CC01-... | Custom |
| Daily History | - | 0x0000CC02-... | Custom |
| Reset Steps | - | 0x0000CC03-... | Custom |
| Pedometer Active | - | 0x0000CC04-... | Custom |
| **Weather Service** | - | 0x0000DD00-... | Custom |
| Temperature | - | 0x0000DD01-... | Custom |
| Weather Icon Code | - | 0x0000DD02-... | Custom |
| Location City | - | 0x0000DD03-... | Custom |
| Latitude | - | 0x0000DD04-... | Custom |
| Longitude | - | 0x0000DD05-... | Custom |
| Weather Update Trigger | - | 0x0000DD06-... | Custom |

**Full Custom UUID Format:**
```
0000XXXX-0000-1000-8000-00805F9B34FB
         ^^^^
         Characteristic code
```

---

## 5. Operation Matrix

### Read Operations
| Service | Characteristic | Implementation | Source |
|---|---|---|---|
| Device Info | All | Hardcoded or from nvs | config.h, qwatch_settings |
| Battery | Level | `time_sync_battery_percent()` | time_sync.cpp |
| Battery | Status | Combined voltage + percent | time_sync.cpp |
| Device Status | Time | `time_sync_get_local_time()` | time_sync.cpp |
| Device Status | Timezone | `weather_service_get_utc_offset_seconds()` | weather_service.cpp |
| Device Status | WiFi SSID | `qwatch_settings_get_wifi_ssid()` | qwatch_settings.cpp |
| Device Status | WiFi Status | `WiFi.status()` | Arduino WiFi |
| Device Status | BT Active | `qwatch_settings_get_bluetooth_active()` | qwatch_settings.cpp |
| Settings | Clock Mode | `qwatch_settings_get_clock_mode()` | qwatch_settings.cpp |
| Settings | Digital Face Index | `qwatch_settings_get_digital_face_index()` | qwatch_settings.cpp |
| Settings | Analogue Face Index | `qwatch_settings_get_analogue_face_index()` | qwatch_settings.cpp |
| Pedometer | Current Steps | `pedometer_get_steps()` | pedometer.cpp |
| Pedometer | Daily History | `pedometer_get_last_7_days()` | pedometer.cpp |
| Pedometer | Active | `qwatch_settings_get_steps_active()` | qwatch_settings.cpp |
| Weather | Temperature | `weather_service_get_temperature_c()` | weather_service.cpp |
| Weather | Icon Code | Internal `s_weather_code` | weather_service.cpp |
| Weather | City/Lat/Lon | Internal location data | weather_service.cpp |

### Write Operations
| Service | Characteristic | Handler | Effect |
|---|---|---|---|
| Device Status | BT Active | `time_sync_set_bluetooth_active()` | Toggle Bluetooth |
| Settings | Clock Mode | `qwatch_settings_set_clock_mode()` + UI update | Switch watch face type |
| Settings | Digital Face Index | `qwatch_settings_set_digital_face_index()` + UI update | Change digital watch face |
| Settings | Analogue Face Index | `qwatch_settings_set_analogue_face_index()` + UI update | Change analogue watch face |
| Settings | WiFi Credentials | Decrypt JSON → `qwatch_settings_set_wifi_ssid/password()` + `qwatch_settings_save_wifi_credentials()` | Store WiFi credentials |
| Settings | Settings Sync Req | Return all settings as JSON | Full settings export |
| Pedometer | Reset Steps | `pedometer_reset()` | Reset step counter |
| Pedometer | Active | `qwatch_settings_set_steps_active()` + `pedometer_start/pause()` | Enable/disable pedometer |
| Weather | Update Trigger | `weather_service_refresh_now()` | Immediate weather sync |

### Notify/Indicate Operations
| Service | Characteristic | Trigger | Frequency | Format |
|---|---|---|---|---|
| Battery | Level | On change or 5s timer | 5s | uint8_t (1 byte) |
| Battery | Status | On change or 5s timer | 5s | JSON or binary (7 bytes) |
| Device Status | Time | 1s timer | 1s | Binary (8 bytes) |
| Device Status | Timezone | On weather sync | 1 hour | int32_t (4 bytes) |
| Device Status | WiFi Status | On connection change | 30s | Binary (3 bytes) |
| Device Status | BT Active | On write or toggle | On change | uint8_t (1 byte) |
| Settings | Clock Mode | On write | On change | uint8_t (1 byte) |
| Pedometer | Current Steps | On step change | 10s | uint32_t (4 bytes) |
| Pedometer | Reset Steps | On write (indicate) | On command | uint8_t (1 byte ack) |
| Weather | Temperature | On weather sync | 1 hour | int8_t (1 byte) |
| Weather | Icon Code | On weather sync | 1 hour | uint16_t (2 bytes) |
| Weather | Update Trigger | On write (indicate) | On command | uint8_t (1 byte ack) |

---

## 6. Message Formats

### 6.1 JSON Message Format (for complex operations)

**Settings Export (on Sync Request):**
```json
{
  "device": {
    "name": "Qwatch",
    "version": "5.7",
    "mac": "AA:BB:CC:DD:EE:FF"
  },
  "clock": {
    "mode": "digital",
    "digital_face": 2,
    "analogue_face": 1,
    "timezone_offset": -18000,
    "current_time": 1686000000
  },
  "connectivity": {
    "wifi_ssid": "MyNetwork",
    "wifi_status": "connected",
    "bluetooth_active": true,
    "battery_percent": 85,
    "battery_voltage": 3950
  },
  "sensors": {
    "pedometer_active": true,
    "current_steps": 5423,
    "temperature_c": 22,
    "weather_code": 3
  }
}
```

**WiFi Credentials (encrypted in transit):**
```json
{
  "ssid": "NetworkName",
  "password": "SecurePassword123",
  "timestamp": 1686000000
}
```

**Pedometer History:**
```json
[
  {"date": "2026-06-11", "steps": 8432},
  {"date": "2026-06-10", "steps": 6521},
  {"date": "2026-06-09", "steps": 7143},
  {"date": "2026-06-08", "steps": 5902},
  {"date": "2026-06-07", "steps": 9234},
  {"date": "2026-06-06", "steps": 6478},
  {"date": "2026-06-05", "steps": 7891}
]
```

### 6.2 Binary Formats (efficient for small messages)

**WiFi Status (3 bytes):**
```c
struct {
  uint8_t status;    // 0=off, 1=connecting, 2=connected, 3=error
  int8_t rssi;       // dBm (-120 to 0)
  uint8_t reserved;  // 0x00
}
```

**Battery Status (7 bytes):**
```c
struct {
  uint8_t percent;       // 0-100
  uint16_t voltage_mv;   // Little-endian (e.g., 0xEE0F = 3950mV)
  uint8_t charging;      // 0=no, 1=yes
  uint8_t health;        // 0=good, 1=warning, 2=critical
  uint16_t reserved;     // 0x0000
}
```

**Current Time (8 bytes):**
```c
struct {
  uint32_t unix_timestamp;  // Little-endian
  uint16_t year;            // Little-endian (e.g., 0xEA07 = 2026)
  uint8_t month;            // 1-12
  uint8_t day;              // 1-31
}
```

---

## 7. Connection Lifecycle

### 7.1 Connection Sequence

```
┌─────────────────────────────────────────────────────────────┐
│ 1. BLE Advertising                                          │
│    - Device: Qwatch                                         │
│    - Services: [6 service UUIDs]                           │
│    - Interval: 100ms                                        │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 2. Companion App Scan & Connect                            │
│    - Scan timeout: 10 seconds                              │
│    - Connect timeout: 30 seconds                           │
│    - App requests pairing if no bond exists                │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 3. MTU Negotiation (Optional)                              │
│    - Request MTU: 517 bytes                                │
│    - Fallback: 23 bytes (default)                          │
│    - Enables efficient bulk transfers                      │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 4. Service Discovery                                        │
│    - GATT client discovers all services                    │
│    - Enables reads/writes/notifications                    │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 5. Enable Notifications (Companion App)                    │
│    - Subscribe to: Battery, Time, Steps, Weather, WiFi     │
│    - Start periodic polling                                │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 6. Synchronization & Operation                             │
│    - Request current state                                 │
│    - Update UI with real-time data                         │
│    - User commands via write operations                    │
└─────────────────────────────────────────────────────────────┘
                          ↓
┌─────────────────────────────────────────────────────────────┐
│ 7. Disconnect Handling                                      │
│    - Graceful disconnect: Stop notifications, close GATT   │
│    - Connection lost: 30s reconnect timeout with backoff   │
│    - State cached locally for offline use                  │
└─────────────────────────────────────────────────────────────┘
```

### 7.2 Pairing & Bonding (Optional for Phase 2)

```
Recommendation: JustWorks pairing (no PIN entry)
- Bonding: Enabled for auto-reconnection
- Encryption: Required for WiFi Credentials characteristic
- Key: 128-bit
- Authentication: 4-digit PIN optional (Phase 3)
```

---

## 8. Existing BLE Capabilities

### ✅ Present in Firmware
- Bluetooth controller initialization (btStart/btStop)
- Enable/disable toggle via `time_sync_set_bluetooth_active()`
- Persistent Bluetooth state in NVS
- WiFi credentials storage (existing LVGL keyboard can be extended)
- JSON parsing library (ArduinoJson) already included
- Persistent settings API (`qwatch_settings`)
- Real-time data APIs (pedometer, weather, battery, time)

### ❌ Absent in Firmware
- **GATT Server implementation**
- **Service definitions**
- **Characteristic handlers**
- **Notification system**
- **MTU negotiation**
- **Pairing/bonding handlers**
- **Encryption support**

---

## 9. Missing BLE Capabilities (Phase 2 Firmware Work)

### 9.1 Firmware Additions Required

1. **BLE GATT Server Setup**
   - Create 6 primary services with 20 characteristics
   - Register read/write/notify callbacks for each
   - Handle MTU negotiation
   - Manage connection state

2. **Notification/Indication System**
   - Timer-based updates (battery 5s, time 1s, pedometer 10s, weather 1h)
   - Event-triggered updates (WiFi status, BT toggle, settings change)
   - Queue management for burst notifications

3. **Encryption Support**
   - AES-128-GCM for WiFi credentials
   - BLE pairing/bonding handlers
   - Key derivation (PBKDF2 from device MAC)

4. **Data Serialization**
   - JSON marshaling (ArduinoJson already present)
   - Binary struct packing for efficient formats
   - Bulk data handling (pedometer history)

---

## 10. Companion App Requirements (Flutter)

### 10.1 BLE Layer
- BLE discovery & scanning
- Connection management with auto-reconnect
- GATT service/characteristic enumeration
- Read/write/notify subscription
- Error handling (disconnects, timeouts)

### 10.2 Data Models
- Watch device metadata
- Settings state (clock mode, face indices, WiFi)
- Pedometer state & history
- Weather & location data
- Battery status

### 10.3 Service Layer
- BLE command execution (write settings, reset pedometer)
- Polling strategy (efficient update rates)
- Caching & offline mode
- Conflict resolution (app vs. watch UI)

### 10.4 UI Screens
- Device discovery & connection
- Real-time status (battery, WiFi, BT)
- Settings editor (clock, face, WiFi)
- Pedometer viewer (chart, history)
- Weather display
- Connection status indicator

### 10.5 State Management
- BLE connection state
- Cached device data
- Pending operations queue
- Error/alert display
- Local vs. remote state reconciliation

---

## 11. Security Considerations

### 11.1 Current Gaps
- WiFi credentials sent over BLE (mitigation: encryption + bonding)
- No device authentication (mitigation: pairing required)
- No API rate limiting (mitigation: app-side throttling)

### 11.2 Recommendations
1. **Mandatory pairing** before any write operations
2. **AES-128-GCM encryption** for WiFi credentials
3. **BLE bonding** to prevent re-pairing
4. **Timeout & backoff** for failed operations
5. **CCMP encryption** at BLE link layer (standard)

---

## 12. Proposed Implementation Timeline

### Phase 2a (Firmware - This Sprint)
- [ ] Create BLE GATT server with 6 services
- [ ] Implement characteristic read/write handlers
- [ ] Add notification callbacks
- [ ] Test discovery & connection
- [ ] Validate message formats

### Phase 2b (Firmware - Next Sprint)
- [ ] Add encryption support (AES-128-GCM)
- [ ] Implement pairing/bonding
- [ ] Optimize MTU negotiation
- [ ] Add error recovery logic

### Phase 3 (Flutter App)
- [ ] Build BLE layer with auto-reconnect
- [ ] Implement data models
- [ ] Create settings UI
- [ ] Integrate state management
- [ ] Test on iOS & Android

---

**PHASE COMPLETE**

