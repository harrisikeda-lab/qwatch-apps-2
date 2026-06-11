# Qwatch Flutter Companion App Architecture

**Status:** Phase 3 Design  
**Date:** 2026-06-11  
**Target:** iOS + Android via Codemagic  
**Framework:** Flutter 3.x with state management

---

## 1. Project Overview

### 1.1 Goals
- Companion app for Qwatch ESP32-S3 watch
- Real-time BLE synchronization
- Settings management (WiFi, clock mode, watch faces)
- Pedometer history visualization
- Weather display with location
- Battery monitoring
- Offline-capable with caching

### 1.2 Technology Stack
- **Language:** Dart 3.x
- **Framework:** Flutter 3.x
- **State Management:** Riverpod 2.x (Provider alternative)
- **BLE Library:** `flutter_reactive_ble` 5.x
- **Local Storage:** `hive` + `hive_flutter`
- **JSON:** `json_serializable`
- **HTTP:** `dio` (for weather API if local sync)
- **Charts:** `fl_chart` 0.65.x
- **UI:** Material 3 design system
- **Build Tool:** Codemagic CI/CD

### 1.3 Platforms
- **iOS:** 13.0+ (requires physical device or simulator for BLE)
- **Android:** API 21+ (Android 5.0)
- **Web:** Not supported (BLE unavailable on web)

---

## 2. Folder Structure

```
qwatch_companion/
├── lib/
│   ├── main.dart                           # App entry point
│   ├── app/
│   │   ├── app.dart                        # App widget, theme, routing
│   │   ├── constants/
│   │   │   ├── app_constants.dart          # App-wide constants
│   │   │   ├── ble_uuids.dart              # BLE UUIDs (services & characteristics)
│   │   │   └── colors_and_styles.dart      # Theme colors, text styles
│   │   └── routes/
│   │       ├── app_routes.dart             # Named routes
│   │       └── route_animations.dart       # Custom page transitions
│   │
│   ├── ble/
│   │   ├── ble_client.dart                 # BLE scanning & connection
│   │   ├── ble_service.dart                # Low-level BLE operations
│   │   ├── ble_device_provider.dart        # Riverpod: Connected device state
│   │   ├── ble_characteristic_reader.dart  # Handle characteristic reads
│   │   ├── ble_characteristic_writer.dart  # Handle characteristic writes
│   │   ├── ble_notification_listener.dart  # Handle subscriptions
│   │   ├── ble_error_handler.dart          # Error recovery & logging
│   │   └── models/
│   │       ├── ble_device_info.dart        # Device metadata
│   │       ├── ble_characteristic.dart     # Characteristic with metadata
│   │       └── ble_connection_state.dart   # Connection state enum
│   │
│   ├── services/
│   │   ├── device_service.dart             # Device info (name, firmware, etc.)
│   │   ├── settings_service.dart           # Clock mode, face indices, WiFi
│   │   ├── pedometer_service.dart          # Steps, history, reset
│   │   ├── weather_service.dart            # Temperature, icon, location
│   │   ├── battery_service.dart            # Battery level, voltage
│   │   ├── connectivity_service.dart       # WiFi/BT status
│   │   ├── sync_service.dart               # Periodic sync manager
│   │   ├── offline_cache_service.dart      # Hive-based local caching
│   │   └── notification_manager.dart       # Push notifications (future)
│   │
│   ├── screens/
│   │   ├── home/
│   │   │   ├── home_screen.dart            # Main dashboard
│   │   │   ├── home_state.dart             # Home screen state (Riverpod)
│   │   │   └── widgets/
│   │   │       ├── device_status_card.dart
│   │   │       ├── battery_indicator.dart
│   │   │       ├── quick_actions.dart
│   │   │       └── sync_status.dart
│   │   │
│   │   ├── device/
│   │   │   ├── device_discovery_screen.dart # Scan & connect
│   │   │   ├── device_list.dart
│   │   │   ├── device_connection_state.dart # Riverpod
│   │   │   └── widgets/
│   │   │       ├── device_tile.dart
│   │   │       ├── connection_indicator.dart
│   │   │       └── scan_progress.dart
│   │   │
│   │   ├── settings/
│   │   │   ├── settings_screen.dart        # Settings editor
│   │   │   ├── settings_state.dart         # Riverpod: Settings mutations
│   │   │   ├── wifi_config_screen.dart     # WiFi credentials
│   │   │   ├── clock_settings_screen.dart  # Clock mode & faces
│   │   │   └── widgets/
│   │   │       ├── toggle_setting.dart
│   │   │       ├── dropdown_setting.dart
│   │   │       └── text_input_setting.dart
│   │   │
│   │   ├── pedometer/
│   │   │   ├── pedometer_screen.dart       # Steps & history
│   │   │   ├── pedometer_state.dart        # Riverpod: History polling
│   │   │   ├── history_chart.dart          # 7-day chart widget
│   │   │   └── widgets/
│   │   │       ├── step_gauge.dart
│   │   │       ├── daily_breakdown.dart
│   │   │       └── reset_button.dart
│   │   │
│   │   ├── weather/
│   │   │   ├── weather_screen.dart         # Weather & location
│   │   │   ├── weather_state.dart          # Riverpod: Weather polling
│   │   │   └── widgets/
│   │   │       ├── weather_card.dart
│   │   │       ├── temperature_display.dart
│   │   │       └── location_info.dart
│   │   │
│   │   ├── about/
│   │   │   └── about_screen.dart           # App info & licenses
│   │   │
│   │   └── error/
│   │       ├── error_screen.dart           # Generic error display
│   │       ├── disconnect_screen.dart      # Connection lost
│   │       └── widgets/
│   │           ├── error_dialog.dart
│   │           └── retry_button.dart
│   │
│   ├── models/
│   │   ├── device.dart                     # Device metadata
│   │   ├── device_settings.dart            # Clock mode, face indices
│   │   ├── pedometer_entry.dart            # Daily step entry
│   │   ├── weather_data.dart               # Temperature, icon, location
│   │   ├── battery_status.dart             # Percent, voltage, charging
│   │   ├── wifi_credentials.dart           # SSID, password (encrypted)
│   │   ├── sync_state.dart                 # Sync status enum
│   │   └── serializers.dart                # Shared JSON serialization
│   │
│   ├── widgets/
│   │   ├── common/
│   │   │   ├── app_bar.dart                # Custom app bar
│   │   │   ├── loading_spinner.dart        # Loading indicator
│   │   │   ├── error_widget.dart           # Error display
│   │   │   ├── empty_state.dart            # No data view
│   │   │   └── action_button.dart          # Custom button
│   │   │
│   │   └── ble/
│   │       ├── ble_permission_request.dart # iOS/Android BLE permissions
│   │       ├── ble_status_indicator.dart   # Connected/disconnected UI
│   │       └── ble_operation_dialog.dart   # Write/sync progress
│   │
│   ├── utils/
│   │   ├── logger.dart                     # Debug logging
│   │   ├── error_handler.dart              # Global error handling
│   │   ├── extensions.dart                 # Dart extensions (String, num, etc.)
│   │   ├── validators.dart                 # Input validation (WiFi, settings)
│   │   ├── formatters.dart                 # Format helpers (date, number)
│   │   ├── date_time_utils.dart            # Date/time manipulation
│   │   ├── encryption_utils.dart           # AES encryption for WiFi credentials
│   │   └── device_info_utils.dart          # Platform info (iOS, Android)
│   │
│   └── providers/
│       ├── ble_providers.dart              # Riverpod: BLE client, connection state
│       ├── device_providers.dart           # Riverpod: Connected device data
│       ├── service_providers.dart          # Riverpod: All services (pedometer, weather, etc.)
│       ├── settings_providers.dart         # Riverpod: Local settings, app preferences
│       ├── sync_providers.dart             # Riverpod: Sync scheduling & state
│       └── cache_providers.dart            # Riverpod: Hive cache access
│
├── test/
│   ├── unit/
│   │   ├── models/
│   │   ├── services/
│   │   └── utils/
│   ├── widget/
│   │   └── screens/
│   └── integration/
│       └── ble_integration_test.dart
│
├── pubspec.yaml                            # Dependencies
├── pubspec.lock
├── analysis_options.yaml                   # Lint rules
├── .github/
│   └── workflows/
│       └── codemagic.yml                   # CI/CD pipeline
├── ios/
│   ├── Runner.xcworkspace/
│   ├── Podfile
│   └── ... (generated by Flutter)
├── android/
│   ├── app/
│   ├── build.gradle
│   └── ... (generated by Flutter)
├── README.md
├── ARCHITECTURE.md
└── CHANGELOG.md
```

---

## 3. Data Flow Architecture

### 3.1 Core Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        Qwatch BLE Device                            │
│  (Real-time data: steps, battery, weather, time, settings state)   │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           │ BLE Notifications & Reads
                           ↓
┌─────────────────────────────────────────────────────────────────────┐
│                      BLE Layer (flutter_reactive_ble)               │
│  ├─ BLE Client (scan, connect, disconnect)                         │
│  ├─ Characteristic Reader (read operations)                        │
│  ├─ Characteristic Writer (write operations)                       │
│  ├─ Notification Listener (subscribe to updates)                   │
│  └─ Error Handler (reconnect, retry, timeout)                      │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           │ Parsed Data (Models)
                           ↓
┌─────────────────────────────────────────────────────────────────────┐
│                      Service Layer                                   │
│  ├─ DeviceService (device info, GATT discovery)                    │
│  ├─ SettingsService (clock mode, face indices, WiFi)               │
│  ├─ PedometerService (steps, history, reset)                       │
│  ├─ WeatherService (temperature, icon, location)                   │
│  ├─ BatteryService (level, voltage, charging)                      │
│  ├─ ConnectivityService (WiFi/BT status)                           │
│  ├─ SyncService (periodic polling & scheduling)                    │
│  └─ OfflineCacheService (Hive local storage)                       │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           │ Riverpod Providers
                           │ (State management)
                           ↓
┌─────────────────────────────────────────────────────────────────────┐
│                    State Management (Riverpod)                      │
│  ├─ bleClientProvider (BLE client instance)                        │
│  ├─ connectedDeviceProvider (current device)                       │
│  ├─ connectionStateProvider (connected/disconnected/error)         │
│  ├─ deviceSettingsProvider (watch settings cache)                  │
│  ├─ pedometerDataProvider (current steps + history)                │
│  ├─ weatherDataProvider (temp, icon, location)                     │
│  ├─ batteryStatusProvider (level, voltage)                         │
│  ├─ syncScheduleProvider (polling intervals)                       │
│  └─ offlineCacheProvider (Hive database access)                    │
└──────────────────────────┬──────────────────────────────────────────┘
                           │
                           │ UI Consumption
                           │ (Riverpod watch() hooks)
                           ↓
┌─────────────────────────────────────────────────────────────────────┐
│                         UI Layer (Screens)                          │
│  ├─ HomeScreen (dashboard, quick actions)                          │
│  ├─ DeviceDiscoveryScreen (scan, connect, select device)           │
│  ├─ SettingsScreen (clock mode, face, WiFi config)                 │
│  ├─ PedometerScreen (steps, 7-day chart)                           │
│  ├─ WeatherScreen (temp, location, icon)                           │
│  ├─ BatteryIndicator (status card)                                 │
│  └─ ErrorScreen (disconnection, sync failure)                      │
└─────────────────────────────────────────────────────────────────────┘
                           │
                           │ User Actions
                           │ (Settings writes, resets)
                           ↓
┌─────────────────────────────────────────────────────────────────────┐
│                   Commands Back to Device                           │
│  ├─ Write settings (clock mode, face index, WiFi creds)            │
│  ├─ Reset pedometer                                                │
│  ├─ Trigger weather refresh                                        │
│  └─ Toggle Bluetooth state                                         │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.2 State Synchronization

```
Watch (Source of Truth)
    │
    ├─ Notification → App receives update
    ├─ App: Update Riverpod provider
    ├─ App: Update Hive cache
    └─ UI: Rebuild with new state

App (User Action)
    │
    ├─ User taps toggle
    ├─ Service writes BLE characteristic
    ├─ Watch: Applies change
    ├─ Watch: Sends notification
    ├─ App: Updates local state
    └─ UI: Reflects new state
```

---

## 4. BLE Layer Architecture

### 4.1 BLE Client Structure

```dart
// ble_client.dart (Singleton)
class BleClient {
  // Responsibilities:
  // 1. Device scanning
  // 2. Connection lifecycle (connect, disconnect, reconnect)
  // 3. MTU negotiation
  // 4. GATT service discovery
  // 5. Underlying flutter_reactive_ble wrapper
  
  Future<List<DiscoveredDevice>> scanForQwatch()
  Future<void> connectToDevice(String deviceId)
  Future<void> disconnectDevice()
  Future<void> negotiateMtu(int mtuSize)
  Future<List<DiscoveredService>> discoverServices()
  Stream<ConnectionStateUpdate> getConnectionStateStream()
}

// ble_service.dart (Facade for operations)
class BleService {
  // Responsibilities:
  // 1. Encapsulate BLE operations
  // 2. Handle characteristic read/write/notify
  // 3. Parse binary/JSON responses
  // 4. Retry logic for failed operations
  // 5. Timeout management
  
  Future<List<int>> readCharacteristic(Uuid service, Uuid char)
  Future<void> writeCharacteristic(Uuid service, Uuid char, List<int> value)
  Stream<List<int>> subscribeToCharacteristic(Uuid service, Uuid char)
  Future<void> unsubscribeFromCharacteristic(Uuid service, Uuid char)
}

// ble_characteristic_reader.dart
class BleCharacteristicReader {
  // Parse raw BLE data into Dart models
  BatteryStatus parseBattery(List<int> bytes)
  Time parseTime(List<int> bytes)
  WiFiStatus parseWiFiStatus(List<int> bytes)
  List<PedometerEntry> parsePedometerHistory(String jsonStr)
  WeatherData parseWeatherData(int temp, int iconCode)
}

// ble_characteristic_writer.dart
class BleCharacteristicWriter {
  // Convert Dart models to BLE-compatible format
  List<int> encodeClockMode(ClockMode mode)
  List<int> encodeFaceIndex(int index)
  List<int> encryptWiFiCredentials(WiFiCredentials creds)
  List<int> encodeResetCommand()
  List<int> encodeWeatherRefreshCommand()
}

// ble_notification_listener.dart
class BleNotificationListener {
  // Subscribe to device notifications
  // Parse updates and notify Riverpod providers
  
  void subscribeToAll(connectedDevice)
  void handleBatteryUpdate(Stream<List<int>>)
  void handleTimeUpdate(Stream<List<int>>)
  void handleStepsUpdate(Stream<List<int>>)
  void handleWeatherUpdate(Stream<List<int>>)
  void handleWiFiStatusUpdate(Stream<List<int>>)
}

// ble_error_handler.dart
class BleErrorHandler {
  // Handle connection errors and recovery
  void handleDisconnection()
  void initiateReconnect(exponentialBackoff)
  void handleTimeout(operationId)
  void handlePermissionDenied()
  void logError(context, error)
}

// ble_device_provider.dart (Riverpod)
@riverpod
class BleDeviceNotifier extends StateNotifier<BleDevice?> {
  // Manages currently connected device
  // Triggers on device selection
  // Persists selection to local storage
}

@riverpod
Stream<ConnectionStateUpdate> bleConnectionState(ref) {
  // Real-time connection status
  // Used by all screens for UI updates
}
```

### 4.2 BLE Operation Lifecycle

```
1. Scan Phase
   - StartScan() → Stream<DiscoveredDevice>
   - Filter: device name == "Qwatch"
   - Timeout: 10 seconds
   - Result: List of Qwatch devices

2. Connection Phase
   - ConnectToDevice(id) 
   - Timeout: 30 seconds
   - MTU negotiation: 517 bytes (fallback: 23)
   - Status: Connecting → Connected/Failed

3. Service Discovery Phase
   - DiscoverServices()
   - GATT database enumeration
   - Status: Discovering → Ready

4. Initial Sync Phase
   - ReadDeviceInfo()
   - ReadCurrentSettings()
   - ReadBatteryStatus()
   - Subscribe to notifications (battery, time, steps, weather)
   - Status: Syncing → Synced

5. Polling Phase (continuous)
   - Timer-based polling (intervals vary by characteristic)
   - Pedometer: 10s
   - Weather: 1h (or on-demand)
   - Battery: 5s
   - Time: 1s (from notification, or read on demand)

6. Disconnect Phase
   - UnsubscribeAll()
   - Disconnect()
   - State: Disconnected
   - Auto-reconnect: Exponential backoff (1s, 2s, 4s, 8s, 30s)
```

---

## 5. State Management (Riverpod)

### 5.1 Provider Hierarchy

```dart
// providers/ble_providers.dart
@riverpod
BleClient bleClient(ref) => BleClient();

@riverpod
BleService bleService(ref) => BleService(ref.watch(bleClientProvider));

@riverpod
class BleConnectionStateNotifier extends StateNotifier<ConnectionState> {
  // ConnectionState enum: disconnected, connecting, connected, error
}

@riverpod
Stream<ConnectionStateUpdate> bleConnectionStream(ref) {
  // Subscribes to flutter_reactive_ble connection updates
}

// providers/device_providers.dart
@riverpod
class ConnectedDeviceNotifier extends StateNotifier<BleDevice?> {
  // Select device → store selection → load cached data
}

@riverpod
Future<DeviceInfo> deviceInfo(ref) {
  // Device metadata (name, firmware, serial)
  // Fetched on connection
  // Cached for offline access
}

// providers/service_providers.dart
@riverpod
class PedometerDataNotifier extends StateNotifier<PedometerData> {
  // Current steps + 7-day history
  // Periodic polling (10s interval)
  // Updates from BLE notifications
}

@riverpod
class WeatherDataNotifier extends StateNotifier<WeatherData?> {
  // Temperature, icon, location
  // Hourly refresh or on-demand
  // Cached in Hive
}

@riverpod
class BatteryStatusNotifier extends StateNotifier<BatteryStatus> {
  // Percent, voltage, charging
  // 5s polling interval
  // Updates from BLE notifications
}

@riverpod
class DeviceSettingsNotifier extends StateNotifier<DeviceSettings> {
  // Clock mode, face indices, WiFi SSID
  // Synced from device on connect
  // Writable via user actions
}

@riverpod
class SyncScheduleNotifier extends StateNotifier<SyncState> {
  // Manages polling intervals
  // Adaptive based on connection quality
  // Cancellable on disconnect
}

// providers/cache_providers.dart
@riverpod
HiveDatabase hiveCache(ref) {
  // Local storage for offline access
  // Stores: device settings, pedometer history, weather data
}

@riverpod
AsyncValue<T> cachedData<T>(ref, String key) {
  // Generic cached data access
  // Async handling for Hive reads
}
```

### 5.2 Watch Pattern Usage in Screens

```dart
// Example: HomeScreen
class HomeScreen extends ConsumerWidget {
  @override
  Widget build(BuildContext context, WidgetRef ref) {
    // Watch multiple providers
    final connectionState = ref.watch(bleConnectionStateProvider);
    final battery = ref.watch(batteryStatusProvider);
    final steps = ref.watch(pedometerDataProvider).currentSteps;
    final weather = ref.watch(weatherDataProvider);
    
    // Rebuild only when these providers change
    // Auto-unsubscribe on widget disposal
    
    return buildUI(connectionState, battery, steps, weather);
  }
}
```

---

## 6. Service Layer Architecture

### 6.1 Service Pattern

```dart
// Abstract base service
abstract class BaseService {
  Future<void> initialize();
  Future<void> sync();
  void dispose();
}

// Device Service - Metadata & Info
class DeviceService extends BaseService {
  // Responsibilities:
  // - Fetch device name, firmware version, MAC address
  // - Cache device info
  // - Handle initial discovery
  
  Future<DeviceInfo> getDeviceInfo()
  Future<bool> verifyConnection()
  Future<void> updateFirmwareInfo()
}

// Settings Service - Configuration
class SettingsService extends BaseService {
  // Responsibilities:
  // - Read/write clock mode
  // - Read/write watch face indices
  // - Read/write WiFi credentials (encrypted)
  // - Apply settings mutations
  
  Future<ClockMode> getClockMode()
  Future<void> setClockMode(ClockMode mode)
  Future<int> getDigitalFaceIndex()
  Future<void> setDigitalFaceIndex(int index)
  Future<void> updateWiFiCredentials(String ssid, String password)
  Future<void> syncAllSettings()
}

// Pedometer Service - Activity Tracking
class PedometerService extends BaseService {
  // Responsibilities:
  // - Poll current steps (10s interval)
  // - Fetch 7-day history
  // - Handle reset command
  // - Listen to step notifications
  
  Future<int> getCurrentSteps()
  Future<List<PedometerEntry>> getHistory(int days)
  Future<void> resetSteps()
  Stream<int> getStepUpdates()
}

// Weather Service - Environmental Data
class WeatherService extends BaseService {
  // Responsibilities:
  // - Fetch temperature, weather code, location
  // - Update hourly or on-demand
  // - Cache in Hive
  
  Future<WeatherData> getWeather()
  Future<void> refreshWeather()
  Stream<WeatherData> getWeatherUpdates()
}

// Battery Service - Power Monitoring
class BatteryService extends BaseService {
  // Responsibilities:
  // - Poll battery level (5s interval)
  // - Fetch voltage and charging status
  // - Alert on low battery
  
  Future<BatteryStatus> getBatteryStatus()
  Stream<BatteryStatus> getBatteryUpdates()
  bool isLowBattery()
}

// Connectivity Service - Network Status
class ConnectivityService extends BaseService {
  // Responsibilities:
  // - Monitor WiFi status
  // - Monitor Bluetooth state
  // - Display connection info
  
  Future<WiFiStatus> getWiFiStatus()
  Future<bool> isBluetoothEnabled()
  Stream<WiFiStatus> getWiFiStatusUpdates()
}

// Sync Service - Orchestration
class SyncService extends BaseService {
  // Responsibilities:
  // - Manage polling intervals
  // - Coordinate multi-service syncs
  // - Handle reconnect logic
  // - Adaptive polling based on device state
  
  Future<void> startSync()
  Future<void> stopSync()
  Future<void> syncAll()
  Future<void> syncPriority(SyncPriority)
  void adjustPollingInterval(Duration)
}

// Offline Cache Service - Persistence
class OfflineCacheService extends BaseService {
  // Responsibilities:
  // - Store/retrieve device data in Hive
  // - Manage cache expiration
  // - Conflict resolution (app vs watch state)
  
  Future<T> get<T>(String key)
  Future<void> set<T>(String key, T value)
  Future<void> invalidate(String key)
  Future<bool> isCacheStale(String key)
  Future<void> clearOldData(Duration maxAge)
}
```

### 6.2 Service Initialization (Riverpod)

```dart
// providers/service_providers.dart

@riverpod
BleDeviceService bleDeviceService(ref) {
  final ble = ref.watch(bleServiceProvider);
  return BleDeviceService(ble);
}

@riverpod
SettingsService settingsService(ref) {
  final ble = ref.watch(bleServiceProvider);
  final cache = ref.watch(hiveProvider);
  return SettingsService(ble, cache);
}

@riverpod
PedometerService pedometerService(ref) {
  final ble = ref.watch(bleServiceProvider);
  final cache = ref.watch(hiveProvider);
  return PedometerService(ble, cache);
}

// ... other services

@riverpod
SyncService syncService(ref) {
  final pedometer = ref.watch(pedometerServiceProvider);
  final weather = ref.watch(weatherServiceProvider);
  final battery = ref.watch(batteryServiceProvider);
  final connectivity = ref.watch(connectivityServiceProvider);
  return SyncService([pedometer, weather, battery, connectivity]);
}
```

---

## 7. Screen Architecture

### 7.1 Home Screen Data Flow

```
HomeScreen (Entry point after device selection)
├── Fetch connection state (Riverpod watch)
├── Display:
│   ├── Device name & status (connected/disconnected)
│   ├── Battery status card
│   │   └── Shows percent, voltage, charging
│   ├── Quick action buttons
│   │   ├── Refresh data
│   │   ├── Settings
│   │   ├── Pedometer
│   │   └── Disconnect
│   ├── Current steps
│   ├── Weather summary
│   ├── Sync status indicator
│   └── Last sync timestamp
└── On user action:
    ├── Settings → Navigate to SettingsScreen
    ├── Refresh → Trigger SyncService.syncAll()
    ├── Disconnect → BleClient.disconnect()
    └── Auto-update every 5s from providers
```

### 7.2 Settings Screen Data Flow

```
SettingsScreen (Configuration)
├── Load current settings (Riverpod watch)
├── Display forms:
│   ├── Clock Mode Toggle (Digital/Analogue)
│   │   └── On change → SettingsService.setClockMode()
│   ├── Digital Face Dropdown
│   │   └── On change → SettingsService.setDigitalFaceIndex()
│   ├── Analogue Face Dropdown
│   │   └── On change → SettingsService.setAnalogueFaceIndex()
│   ├── WiFi SSID Text Input
│   │   └── Validation: length, charset
│   ├── WiFi Password Text Input
│   │   └── Validation: length, charset
│   └── Save Button
│       └── On tap → SettingsService.updateWiFiCredentials()
│
└── Error handling:
    ├── Invalid input → Show snackbar
    ├── BLE write failed → Retry with backoff
    ├── Timeout → User prompt to reconnect
    └── Success → Confirmation & sync cache
```

### 7.3 Pedometer Screen Data Flow

```
PedometerScreen (Activity Tracking)
├── Watch providers:
│   ├── Current steps (real-time)
│   ├── 7-day history
│   └── Pedometer active state
│
├── Display:
│   ├── Large steps gauge/counter
│   ├── 7-day bar chart (fl_chart)
│   │   └── Y-axis: steps, X-axis: dates
│   ├── Daily breakdown (table view)
│   ├── Stats:
│   │   ├── Today's total
│   │   ├── Weekly average
│   │   ├── Peak day
│   │   └── Lowest day
│   └── Reset button (with confirmation)
│
└── Interactions:
    ├── Tap day on chart → Show daily detail
    ├── Tap reset → Confirm dialog → PedometerService.resetSteps()
    ├── Swipe refresh → PedometerService.sync()
    └── Auto-update every 10s from notifications
```

### 7.4 Weather Screen Data Flow

```
WeatherScreen (Environmental Data)
├── Watch provider: WeatherData
│
├── Display:
│   ├── Weather icon (from emoji map)
│   ├── Temperature (large, °C)
│   ├── Location (city)
│   ├── Coordinates (lat/lon)
│   ├── Last update timestamp
│   └── Refresh button
│
└── Interactions:
    ├── Tap refresh → WeatherService.refreshWeather()
    ├── Auto-update on 1h interval
    └── Cache invalidation & fallback to last known
```

### 7.5 Device Discovery Screen Data Flow

```
DeviceDiscoveryScreen (Scan & Connect)
├── Permission checks (iOS: location, Android: scan)
│   └── If denied → Show permission dialog
│
├── Scan phase:
│   ├── BleClient.scanForQwatch()
│   ├── Filter: device name contains "Qwatch"
│   ├── Display in list with:
│   │   ├── Device name
│   │   ├── RSSI (signal strength)
│   │   └── Tap to connect button
│   └── Timeout: 10 seconds → Auto-stop
│
├── Connection phase:
│   ├── On device tap → BleClient.connectToDevice(id)
│   ├── Show loading spinner
│   ├── Timeout: 30 seconds
│   ├── If success:
│   │   ├── Store device selection
│   │   ├── Trigger initial sync
│   │   └── Navigate to HomeScreen
│   └── If failed:
│       ├── Show error dialog
│       ├── Retry option
│       └── Stay on discovery screen
│
└── Error handling:
    ├── Bluetooth disabled → Prompt to enable
    ├── Permissions denied → Show settings link
    ├── Device not found → Retry scan
    └── Connection timeout → Show troubleshooting
```

---

## 8. Error Handling Strategy

### 8.1 Error Classification

```
BLE-Level Errors:
├── ConnectionError
│   ├── DeviceNotFound
│   ├── Timeout
│   ├── AlreadyConnected
│   ├── PermissionDenied
│   └── BluetoothDisabled
├── CharacteristicError
│   ├── NotFound
│   ├── NotReadable
│   ├── NotWritable
│   └── OperationFailed
└── TimeoutError
    └── Retry logic with exponential backoff

Service-Level Errors:
├── ValidationError (input validation)
├── ParseError (malformed BLE data)
├── EncryptionError (WiFi credentials)
├── CacheError (Hive access)
└── SyncError (polling failure)

UI-Level Errors:
├── ConnectionLost
├── DeviceDisconnected
├── SyncFailed
├── InvalidSettings
└── Unknown
```

### 8.2 Global Error Handler

```dart
// utils/error_handler.dart
class AppErrorHandler {
  static Future<void> handle(dynamic error, StackTrace stack) {
    // 1. Log to console/file
    Logger.error(error, stack);
    
    // 2. Classify error
    final classification = classify(error);
    
    // 3. Decide action
    switch (classification) {
      case ErrorType.connectionLost:
        // Trigger reconnect
        triggerAutoReconnect();
        break;
      case ErrorType.timeout:
        // Retry with backoff
        retryWithExponentialBackoff();
        break;
      case ErrorType.validation:
        // Show user-friendly message
        showValidationError();
        break;
      case ErrorType.permission:
        // Show permission request
        showPermissionDialog();
        break;
      default:
        // Generic error screen
        showErrorScreen();
    }
  }
  
  static Future<void> retryWithExponentialBackoff({
    int maxRetries = 5,
    Duration initialDelay = const Duration(milliseconds: 500),
  }) {
    // Implement exponential backoff
    // 500ms, 1s, 2s, 4s, 8s
  }
}
```

### 8.3 Connection Reconnect Logic

```
Disconnect Event
    ↓
ConnectionState → Disconnected
    ↓
1st Reconnect Attempt (500ms delay)
    │
    ├─ Success → Connected (UI updates)
    └─ Failure
        ↓
2nd Reconnect Attempt (1s delay)
    │
    ├─ Success → Connected
    └─ Failure
        ↓
3rd Reconnect Attempt (2s delay)
    │
    ├─ Success → Connected
    └─ Failure
        ↓
4th Reconnect Attempt (4s delay)
    │
    ├─ Success → Connected
    └─ Failure
        ↓
5th Reconnect Attempt (8s delay)
    │
    ├─ Success → Connected
    └─ Failure
        ↓
6th Reconnect Attempt (30s delay)
    │
    ├─ Success → Connected
    └─ Failure → Give up, show error screen
        
Max total time: ~47 seconds before user intervention required
```

---

## 9. Offline & Caching Strategy

### 9.1 Hive Schema

```dart
// models/hive_schemas.dart

@HiveType(typeId: 0)
class CachedDeviceInfo {
  @HiveField(0) String id;
  @HiveField(1) String name;
  @HiveField(2) String firmware;
  @HiveField(3) DateTime fetchedAt;
}

@HiveType(typeId: 1)
class CachedDeviceSettings {
  @HiveField(0) ClockMode clockMode;
  @HiveField(1) int digitalFaceIndex;
  @HiveField(2) int analogueFaceIndex;
  @HiveField(3) String? wifiSsid;
  @HiveField(4) DateTime lastSyncedAt;
}

@HiveType(typeId: 2)
class CachedPedometerHistory {
  @HiveField(0) List<PedometerEntry> entries;
  @HiveField(1) DateTime fetchedAt;
}

@HiveType(typeId: 3)
class CachedWeatherData {
  @HiveField(0) int tempC;
  @HiveField(1) int iconCode;
  @HiveField(2) String? city;
  @HiveField(3) double? latitude;
  @HiveField(4) double? longitude;
  @HiveField(5) DateTime fetchedAt;
}
```

### 9.2 Cache Strategies

```
Strategy 1: Network-First (Online preference)
├─ Try BLE read → Success: Update cache & return
├─ Try BLE read → Failure: Return cached value (if exists)
└─ No cache: Return error

Strategy 2: Cache-First (Offline preference)
├─ Check cache freshness (< 5 min)
├─ If fresh: Return immediately
├─ If stale: Try BLE update
└─ If BLE fails: Return stale cache

Strategy 3: Adaptive (Connection quality aware)
├─ If device connected & good signal:
│   └─ Network-first approach
├─ If device connected & poor signal:
│   └─ Increase cache TTL, reduce polling
└─ If device disconnected:
    └─ Cache-first approach

Cache TTL (Time-to-live):
├─ Device info: 1 hour
├─ Device settings: 5 minutes
├─ Battery status: 1 minute
├─ Pedometer history: 10 minutes
├─ Weather data: 1 hour
└─ Last sync: Always fresh
```

---

## 10. Riverpod Provider Organization

### 10.1 Provider File Structure

```dart
// lib/providers/ble_providers.dart
@riverpod
BleClient bleClient(ref) => BleClient();

@riverpod
BleService bleService(ref) => BleService(ref.watch(bleClientProvider));

@riverpod
class ConnectionStateNotifier extends StateNotifier<ConnectionState> { }

@riverpod
Stream<ConnectionStateUpdate> connectionStream(ref) { }

// lib/providers/device_providers.dart
@riverpod
class ConnectedDeviceNotifier extends StateNotifier<BleDevice?> { }

@riverpod
Future<DeviceInfo> deviceInfo(ref) { }

// lib/providers/service_providers.dart
@riverpod
BleDeviceService deviceService(ref) { }

@riverpod
SettingsService settingsService(ref) { }

@riverpod
PedometerService pedometerService(ref) { }

// lib/providers/data_providers.dart
@riverpod
class PedometerDataNotifier extends StateNotifier<PedometerData> { }

@riverpod
class WeatherDataNotifier extends StateNotifier<WeatherData?> { }

@riverpod
class BatteryStatusNotifier extends StateNotifier<BatteryStatus> { }

// lib/providers/sync_providers.dart
@riverpod
class SyncStateNotifier extends StateNotifier<SyncState> { }

@riverpod
SyncService syncService(ref) { }

// lib/providers/cache_providers.dart
@riverpod
HiveDatabase hiveDatabase(ref) { }
```

### 10.2 Provider Dependencies

```
bleClient (no deps)
    ↓
bleService (depends on bleClient)
    ├─ deviceService (depends on bleService)
    ├─ settingsService (depends on bleService + hiveDatabase)
    ├─ pedometerService (depends on bleService + hiveDatabase)
    ├─ weatherService (depends on bleService + hiveDatabase)
    ├─ batteryService (depends on bleService)
    └─ connectivityService (depends on bleService)
        ↓
    syncService (depends on all services above)
        ↓
    Device screens (watch providers for UI updates)
```

---

## 11. Testing Strategy

### 11.1 Unit Tests

```dart
// test/unit/models/device_test.dart
- Device serialization/deserialization
- Model validation

// test/unit/services/settings_service_test.dart
- Settings mutation logic
- Cache validation
- Encryption/decryption

// test/unit/utils/encryption_utils_test.dart
- WiFi credentials encryption
- Key derivation
```

### 11.2 Widget Tests

```dart
// test/widget/screens/home_screen_test.dart
- Home screen rendering
- Provider watch behavior
- Button interactions

// test/widget/screens/pedometer_screen_test.dart
- Chart rendering
- History display
- Reset button logic
```

### 11.3 Integration Tests

```dart
// test/integration/ble_integration_test.dart
- Real BLE device connection
- Full data flow: discover → connect → sync → update UI
- Error recovery
- Offline fallback
```

---

## 12. Build & Deployment (Codemagic)

### 12.1 Codemagic Configuration

```yaml
# .github/workflows/codemagic.yml
workflows:
  android:
    script:
      - flutter build apk --release
      - gsutil cp build/app/outputs/apk/release/app-release.apk gs://...
  
  ios:
    script:
      - flutter build ios --release
      - xcodebuild archive -workspace ios/Runner.xcworkspace -scheme Runner -archivePath build/ios/archive.xcarchive
      - xcodebuild -exportArchive -archivePath build/ios/archive.xcarchive -exportOptionsPlist build/ios/options.plist -exportPath build/ios/ipa

  # Automatic builds on push to main
  trigger: on-push
  branch: main

  # Environment variables
  env:
    FLUTTER_VERSION: 3.x.x
    XCODE_VERSION: latest
```

### 12.2 Platform-Specific Configuration

**iOS (ios/Runner/Info.plist):**
```xml
<key>NSBluetoothPeripheralUsageDescription</key>
<string>This app needs Bluetooth to connect to your Qwatch</string>
<key>NSLocationWhenInUseUsageDescription</key>
<string>Location is used for weather information</string>
```

**Android (android/app/AndroidManifest.xml):**
```xml
<uses-permission android:name="android.permission.BLUETOOTH" />
<uses-permission android:name="android.permission.BLUETOOTH_ADMIN" />
<uses-permission android:name="android.permission.BLUETOOTH_SCAN" />
<uses-permission android:name="android.permission.BLUETOOTH_CONNECT" />
<uses-permission android:name="android.permission.ACCESS_FINE_LOCATION" />
```

---

## 13. Dependency Management

### 13.1 pubspec.yaml Structure

```yaml
dependencies:
  flutter:
    sdk: flutter
  
  # State Management
  riverpod: ^2.0.0
  flutter_riverpod: ^2.0.0
  hooks_riverpod: ^2.0.0
  
  # BLE
  flutter_reactive_ble: ^5.0.0
  
  # Local Storage
  hive: ^2.0.0
  hive_flutter: ^1.0.0
  
  # JSON
  json_serializable: ^6.0.0
  
  # HTTP (optional, for fallback weather)
  dio: ^5.0.0
  
  # UI & Charts
  fl_chart: ^0.65.0
  
  # Logging
  logger: ^2.0.0

dev_dependencies:
  flutter_test:
    sdk: flutter
  
  build_runner: ^2.0.0
  riverpod_generator: ^2.0.0
  json_serializable: ^6.0.0
```

---

## 14. Production Readiness Checklist

- [ ] BLE layer: Scan, connect, disconnect, error recovery
- [ ] Services: Device, settings, pedometer, weather, battery, sync
- [ ] State management: Riverpod providers with proper dependencies
- [ ] Screens: Home, device discovery, settings, pedometer, weather
- [ ] Error handling: Global error handler, reconnect logic, timeouts
- [ ] Caching: Hive schemas, TTL, offline mode
- [ ] Testing: Unit, widget, integration tests
- [ ] iOS: Permissions, signing, App Store build
- [ ] Android: Permissions, signing, Google Play build
- [ ] Codemagic: CI/CD pipeline, automated builds & deployment
- [ ] Documentation: API docs, user guide, troubleshooting
- [ ] Security: WiFi credentials encryption, pairing requirement
- [ ] Monitoring: Crash reporting (Firebase), analytics
- [ ] Performance: BLE MTU optimization, polling interval tuning

---

**PHASE COMPLETE**

