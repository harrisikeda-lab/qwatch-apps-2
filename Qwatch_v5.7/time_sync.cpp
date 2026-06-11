#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <esp32-hal-bt.h>

#include "config.h"
#include "qwatch_settings.h"
#include "time_sync.h"
#include "weather_service.h"

// We consider time "valid" once system time is set beyond a sane epoch.
static bool s_time_valid = false;
static bool s_wifi_active = true;

// Tracks whether we've already done the scheduled daily sync attempt today.
static int s_last_scheduled_sync_year = -1;
static int s_last_scheduled_sync_yday = -1;

// Tracks whether we have already attempted the hourly weather sync this hour.
static int s_last_weather_sync_year = -1;
static int s_last_weather_sync_yday = -1;
static int s_last_weather_sync_hour = -1;

// ---- helpers ----
static void apply_weather_timezone() {
  configTime(
    weather_service_get_utc_offset_seconds(),
    0,
    NTP_SERVER_1,
    NTP_SERVER_2,
    NTP_SERVER_3
  );
}

static bool fetch_time(uint32_t timeout_ms) {
  apply_weather_timezone();

  struct tm t;
  const uint32_t start = millis();
  while ((millis() - start) < timeout_ms) {
    if (getLocalTime(&t, 200)) {
      time_t now;
      time(&now);
      if (now > 1672531200) {
        return true;
      }
    }
    delay(50);
  }
  return false;
}

static void disconnect_wifi() {
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);
}

static bool handle_wifi_connected_event() {
  const bool weather_ok = weather_service_refresh_now();

  const uint32_t ntp_timeout = 12000;
  const bool time_ok = fetch_time(ntp_timeout);

  if (time_ok) {
    s_time_valid = true;

    if (weather_ok) {
      struct tm t;
      if (getLocalTime(&t, 100)) {
        s_last_weather_sync_year = t.tm_year;
        s_last_weather_sync_yday = t.tm_yday;
        s_last_weather_sync_hour = t.tm_hour;
      }
    }
  }

  return weather_ok && time_ok;
}

static bool connect_wifi(
  uint32_t timeout_ms,
  bool *out_connected_now = nullptr,
  bool ignore_wifi_active = false
) {
  if (out_connected_now) {
    *out_connected_now = false;
  }

  if (!ignore_wifi_active && !s_wifi_active) {
    return false;
  }

  if (!qwatch_settings_has_wifi_credentials()) {
    return false;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.persistent(false);

  // Cleanly reset STA state before attempting a fresh join.
  WiFi.disconnect(false, true);
  delay(100);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(
    qwatch_settings_get_wifi_ssid(),
    qwatch_settings_get_wifi_password()
  );

  const uint32_t start = millis();
  while (
    WiFi.status() != WL_CONNECTED &&
    (millis() - start) < timeout_ms
  ) {
    delay(50);
  }

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.setSleep(true);

    if (out_connected_now) {
      *out_connected_now = true;
    }

    return true;
  }

  return false;
}

static bool boot_sync_once() {
  const uint32_t wifi_timeout = 12000;

  if (!s_wifi_active) {
    return false;
  }

  if (!qwatch_settings_has_wifi_credentials()) {
    return false;
  }

  bool connected_now = false;
  if (!connect_wifi(wifi_timeout, &connected_now)) {
    disconnect_wifi();
    return false;
  }

  return handle_wifi_connected_event();
}

static void attempt_boot_sync() {
  static constexpr int BOOT_SYNC_ATTEMPTS = 3;

  for (int attempt = 1; attempt <= BOOT_SYNC_ATTEMPTS; ++attempt) {
    Serial.print("[time_sync] Boot Wi-Fi/time/weather sync attempt ");
    Serial.print(attempt);
    Serial.print(" of ");
    Serial.println(BOOT_SYNC_ATTEMPTS);

    if (boot_sync_once()) {
      Serial.println(
        "[time_sync] Boot time/weather sync succeeded; turning Wi-Fi off"
      );
      disconnect_wifi();
      return;
    }

    Serial.println("[time_sync] Boot time/weather sync attempt failed");

    if (attempt < BOOT_SYNC_ATTEMPTS) {
      delay(500);
    }
  }

  Serial.println(
    "[time_sync] Boot sync failed after 3 attempts; turning Wi-Fi off"
  );
  disconnect_wifi();
}

static bool get_local_time_now(struct tm *out_tm) {
  if (!out_tm) {
    return false;
  }
  return getLocalTime(out_tm, 100);
}

static bool already_attempted_scheduled_sync_today(const struct tm &t) {
  return (s_last_scheduled_sync_year == t.tm_year) &&
         (s_last_scheduled_sync_yday == t.tm_yday);
}

static void mark_scheduled_sync_attempt_today(const struct tm &t) {
  s_last_scheduled_sync_year = t.tm_year;
  s_last_scheduled_sync_yday = t.tm_yday;
}

static bool already_attempted_weather_sync_this_hour(const struct tm &t) {
  return (s_last_weather_sync_year == t.tm_year) &&
         (s_last_weather_sync_yday == t.tm_yday) &&
         (s_last_weather_sync_hour == t.tm_hour);
}

static void mark_weather_sync_attempt_this_hour(const struct tm &t) {
  s_last_weather_sync_year = t.tm_year;
  s_last_weather_sync_yday = t.tm_yday;
  s_last_weather_sync_hour = t.tm_hour;
}

static bool is_on_the_hour(const struct tm &t) {
  return t.tm_min == 0;
}

static bool is_at_or_past_scheduled_sync_time(const struct tm &t) {
  if (t.tm_hour != NTP_SYNC_HOUR) {
    return t.tm_hour > NTP_SYNC_HOUR;
  }

  if (t.tm_min != NTP_SYNC_MINUTE) {
    return t.tm_min > NTP_SYNC_MINUTE;
  }

  return t.tm_sec >= NTP_SYNC_SECOND;
}

static bool configured_network_available() {
  if (!s_wifi_active) {
    return false;
  }

  if (!qwatch_settings_has_wifi_credentials()) {
    return false;
  }

  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  // Skip Wi-Fi scanning because it blocks on wake.
  // Assume the configured network may be available and let the
  // actual connection attempt determine success or failure.
  return true;
}

static void attempt_sync(const char *reason) {
  (void)reason;

  if (!s_wifi_active) {
    return;
  }

  if (!qwatch_settings_has_wifi_credentials()) {
    return;
  }

  const uint32_t wifi_timeout = 12000;
  bool connected_now = false;

  if (!connect_wifi(wifi_timeout, &connected_now)) {
    disconnect_wifi();
    return;
  }

  // If Wi-Fi was already connected, we still want the scheduled/automatic sync
  // path to perform the normal post-connect actions, including time sync.
  handle_wifi_connected_event();

  // Automatic syncs should not leave the radio powered after they complete.
  // This deliberately does not set s_wifi_active = false, so future automatic
  // syncs can turn Wi-Fi back on again.
  disconnect_wifi();
}

static bool attempt_hourly_weather_sync_once() {
  const uint32_t wifi_timeout = 12000;

  if (!qwatch_settings_has_wifi_credentials()) {
    return false;
  }

  bool connected_now = false;
  if (!connect_wifi(wifi_timeout, &connected_now, true)) {
    disconnect_wifi();
    return false;
  }

  const bool ok = weather_service_refresh_now();
  if (ok) {
    // Apply the newly returned UTC offset immediately. This keeps the
    // displayed local time correct after DST transitions or travel.
    apply_weather_timezone();
  }

  disconnect_wifi();
  return ok;
}

static void attempt_hourly_weather_sync(const struct tm &t) {
  static constexpr int WEATHER_SYNC_ATTEMPTS = 3;

  if (!is_on_the_hour(t)) {
    return;
  }

  if (already_attempted_weather_sync_this_hour(t)) {
    return;
  }

  // Mark before attempting so a failed three-attempt cycle is not repeated
  // continuously for the whole first minute of the hour.
  mark_weather_sync_attempt_this_hour(t);

  if (!qwatch_settings_has_wifi_credentials()) {
    Serial.println(
      "[time_sync] Hourly weather sync skipped: no Wi-Fi credentials"
    );
    return;
  }

  for (int attempt = 1; attempt <= WEATHER_SYNC_ATTEMPTS; ++attempt) {
    Serial.print("[time_sync] Hourly weather sync attempt ");
    Serial.print(attempt);
    Serial.print(" of ");
    Serial.println(WEATHER_SYNC_ATTEMPTS);

    if (attempt_hourly_weather_sync_once()) {
      Serial.println(
        "[time_sync] Hourly weather sync succeeded; Wi-Fi off"
      );
      return;
    }

    Serial.println("[time_sync] Hourly weather sync attempt failed");

    if (attempt < WEATHER_SYNC_ATTEMPTS) {
      delay(500);
    }
  }

  Serial.println(
    "[time_sync] Hourly weather sync failed after 3 attempts; Wi-Fi off"
  );
}

static bool set_bluetooth_controller_active(bool active) {
  if (active) {
    if (btStarted()) {
      return true;
    }
    return btStart();
  }

  if (!btStarted()) {
    return true;
  }
  return btStop();
}

// ---- battery helpers ----
// Waveshare board:
// VBAT -> 200K / 100K divider -> GPIO1
// Official formula:
//   voltage = 3.3 / (1<<12) * 3 * AD_Value
//
// Use raw ADC readings here, not analogReadMilliVolts(), because the board's
// own reference design and example formula are based on raw 12-bit ADC counts.
static constexpr float ADC_REF_VOLTAGE = 3.3f;
static constexpr float ADC_FULL_SCALE  = 4096.0f;
static constexpr float BATTERY_DIVIDER_RATIO = 3.0f;
static constexpr float BATTERY_VOLTAGE_CALIBRATION = 1.1333f;
static constexpr uint32_t BATTERY_CACHE_MS = 5000UL;

// Display scaling only:
// keep the raw voltage calculation exactly as before, but map "full" a little
// earlier so a fully charged battery on the UI does not sit one bar down.
static constexpr float BATTERY_EMPTY_VOLTAGE = 3.30f;
static constexpr float BATTERY_FULL_VOLTAGE  = 4.00f;

static float s_cached_battery_voltage = 0.0f;
static uint32_t s_last_battery_read_ms = 0;

static float sample_battery_voltage() {
  uint32_t total = 0;

  for (int i = 0; i < BATTERY_ADC_SAMPLES; ++i) {
    total += analogRead(BAT_ADC_PIN);
    delay(2);
  }

  const float raw = (float)total / (float)BATTERY_ADC_SAMPLES;
  return (ADC_REF_VOLTAGE / ADC_FULL_SCALE) *
         BATTERY_DIVIDER_RATIO *
         raw *
         BATTERY_VOLTAGE_CALIBRATION;
}

static float read_battery_voltage_raw() {
  const uint32_t now = millis();

  if (
    s_cached_battery_voltage <= 0.0f ||
    (uint32_t)(now - s_last_battery_read_ms) >= BATTERY_CACHE_MS
  ) {
    s_cached_battery_voltage = sample_battery_voltage();
    s_last_battery_read_ms = now;
  }

  return s_cached_battery_voltage;
}

static int battery_percent_from_voltage(float vbat) {
  if (vbat >= BATTERY_FULL_VOLTAGE) return 100;
  if (vbat <= BATTERY_EMPTY_VOLTAGE) return 0;

  const float pct =
      ((vbat - BATTERY_EMPTY_VOLTAGE) /
       (BATTERY_FULL_VOLTAGE - BATTERY_EMPTY_VOLTAGE)) * 100.0f;

  return (int)(pct + 0.5f);
}

// ---- public API ----
void time_sync_init() {
  analogReadResolution(12);
  analogSetPinAttenuation(BAT_ADC_PIN, ADC_11db);

  set_bluetooth_controller_active(
    qwatch_settings_get_bluetooth_active()
  );

  // Boot behaviour:
  // 1. Turn Wi-Fi on.
  // 2. Try to connect using credentials already loaded into qwatch_settings.
  // 3. Sync weather and time.
  // 4. Make up to 3 attempts.
  // 5. Turn Wi-Fi off afterwards to save power.
  attempt_boot_sync();
}

void time_sync_loop() {
  if (!s_time_valid) {
    static uint32_t last_try = 0;
    if (millis() - last_try > 30000UL) {
      last_try = millis();
      attempt_sync("initial");
    }
    return;
  }

  struct tm now_local;
  if (!get_local_time_now(&now_local)) {
    return;
  }

  if (
    !already_attempted_scheduled_sync_today(now_local) &&
    is_at_or_past_scheduled_sync_time(now_local)
  ) {
    // Count the day as "done" whether the network is available or not,
    // so we do not keep checking repeatedly for the rest of that day.
    mark_scheduled_sync_attempt_today(now_local);

    // Do a network availability check first. If the configured network
    // is not present, do nothing and just keep the current time running.
    if (configured_network_available()) {
      attempt_sync("scheduled-daily");
    }
  }

  attempt_hourly_weather_sync(now_local);
}
bool time_sync_get_local_time(struct tm *out_tm) {
  if (!s_time_valid || !out_tm) return false;
  return getLocalTime(out_tm, 100);
}
bool time_sync_format_date(char *out, int out_len) {
  if (!s_time_valid) return false;

  struct tm t;
  if (!getLocalTime(&t, 100)) return false;

  strftime(out, out_len, "%a %d %b", &t);
  return true;
}

bool time_sync_wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

bool time_sync_wifi_is_active() {
  return s_wifi_active && WiFi.getMode() != WIFI_OFF;
}

bool time_sync_set_wifi_active(bool active) {
  if (!active) {
    s_wifi_active = false;
    disconnect_wifi();
    return true;
  }

  s_wifi_active = true;

  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  if (connect_wifi(12000)) {
    handle_wifi_connected_event();
  }

  return true;
}

bool time_sync_set_bluetooth_active(bool active) {
  const bool ok = set_bluetooth_controller_active(active);
  if (ok) {
    qwatch_settings_set_bluetooth_active(active);
  }
  return ok;
}

bool time_sync_bluetooth_is_active() {
  return btStarted();
}
float time_sync_battery_voltage() {
  return read_battery_voltage_raw();
}

int time_sync_battery_percent() {
  return battery_percent_from_voltage(
    read_battery_voltage_raw()
  );
}
