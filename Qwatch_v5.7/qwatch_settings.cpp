#include "qwatch_settings.h"

#include <Arduino.h>
#include <FFat.h>
#include <string.h>

#include "config.h"
#include "watch_faces.h"

namespace {
ClockMode g_clock_mode = CLOCK_MODE_DIGITAL;
bool g_steps_active = false;
bool g_bluetooth_active = false;
int g_digital_face_index = 0;
int g_analogue_face_index = 0;

String g_wifi_ssid;
String g_wifi_password;

// Shared module-local view of whether FFat was mounted successfully at boot.
// This is now established once in qwatch_settings_init(), which runs before
// time_sync_init() and before pedometer_init().
bool g_fs_ready = false;

constexpr const char* WIFI_CREDENTIALS_FILE = "/wifi_credentials.csv";
constexpr const char* WATCH_FACE_SETTINGS_FILE = "/watch_face_settings.csv";

int clamp_index(int index, int count) {
  if (count <= 0) return 0;
  if (index < 0) return 0;
  if (index >= count) return count - 1;
  return index;
}

bool ensure_fs_ready() {
  return g_fs_ready;
}

bool config_wifi_credentials_present() {
  return strlen(QWATCH_WIFI_SSID) > 0;
}

bool save_watch_face_settings() {
  if (!ensure_fs_ready()) {
    Serial.println("[qwatch_settings] Watch face save failed: FFat not ready");
    return false;
  }

  File f = FFat.open(WATCH_FACE_SETTINGS_FILE, FILE_WRITE);
  if (!f) {
    Serial.println("[qwatch_settings] Watch face save failed: could not open /watch_face_settings.csv for write");
    return false;
  }

  f.print((int)g_clock_mode);
  f.print(",");
  f.print(g_digital_face_index);
  f.print(",");
  f.println(g_analogue_face_index);
  f.close();

  Serial.print("[qwatch_settings] Saved watch face settings: mode=");
  Serial.print((int)g_clock_mode);
  Serial.print(", digital=");
  Serial.print(g_digital_face_index);
  Serial.print(", analogue=");
  Serial.println(g_analogue_face_index);
  return true;
}

bool load_watch_face_settings() {
  if (!ensure_fs_ready()) {
    Serial.println("[qwatch_settings] Watch face load failed: FFat not ready");
    return false;
  }

  File f = FFat.open(WATCH_FACE_SETTINGS_FILE, FILE_READ);
  if (!f) {
    Serial.println("[qwatch_settings] No saved watch face settings found; using defaults");
    return false;
  }

  String line = f.readStringUntil('\n');
  f.close();
  line.trim();

  const int first_comma = line.indexOf(',');
  const int second_comma = line.indexOf(',', first_comma + 1);
  if (line.length() == 0 || first_comma < 0 || second_comma < 0) {
    Serial.println("[qwatch_settings] Watch face load failed: malformed settings file");
    return false;
  }

  const int mode = line.substring(0, first_comma).toInt();
  const int digital_index = line.substring(first_comma + 1, second_comma).toInt();
  const int analogue_index = line.substring(second_comma + 1).toInt();

  if (mode != CLOCK_MODE_DIGITAL && mode != CLOCK_MODE_ANALOGUE) {
    Serial.println("[qwatch_settings] Watch face load failed: invalid clock mode");
    return false;
  }

  g_clock_mode = (ClockMode)mode;
  g_digital_face_index = clamp_index(digital_index, watch_faces_get_digital_count());
  g_analogue_face_index = clamp_index(analogue_index, watch_faces_get_analogue_count());

  Serial.print("[qwatch_settings] Loaded watch face settings: mode=");
  Serial.print((int)g_clock_mode);
  Serial.print(", digital=");
  Serial.print(g_digital_face_index);
  Serial.print(", analogue=");
  Serial.println(g_analogue_face_index);
  return true;
}
}

void qwatch_settings_init() {
  g_clock_mode = CLOCK_MODE_DIGITAL;
  g_steps_active = false;
  g_bluetooth_active = false;
  g_digital_face_index = 0;
  g_analogue_face_index = 0;
  g_wifi_ssid = "";
  g_wifi_password = "";

  // Mount FFat once here, early in boot, using the same pattern pedometer.cpp
  // already uses privately.
  g_fs_ready = FFat.begin(true);

  Serial.print("[qwatch_settings] FFat.begin(true): ");
  Serial.println(g_fs_ready ? "SUCCESS" : "FAILED");

  if (!g_fs_ready) {
    Serial.println("[qwatch_settings] FFat not ready; settings cannot be loaded or saved");
    return;
  }

  load_watch_face_settings();

  if (config_wifi_credentials_present()) {
    Serial.println("[qwatch_settings] Using Wi-Fi credentials from config.h");

    g_wifi_ssid = QWATCH_WIFI_SSID;
    g_wifi_password = QWATCH_WIFI_PASSWORD;

    const bool save_ok = qwatch_settings_save_wifi_credentials();
    Serial.print("[qwatch_settings] qwatch_settings_save_wifi_credentials() from config.h: ");
    Serial.println(save_ok ? "SUCCESS" : "FAILED");
    return;
  }

  const bool load_ok = qwatch_settings_load_wifi_credentials();
  Serial.print("[qwatch_settings] qwatch_settings_load_wifi_credentials(): ");
  Serial.println(load_ok ? "SUCCESS" : "FAILED");

}

void qwatch_settings_set_clock_mode(ClockMode mode) {
  if (mode != CLOCK_MODE_DIGITAL && mode != CLOCK_MODE_ANALOGUE) {
    return;
  }

  if (g_clock_mode == mode) {
    return;
  }

  g_clock_mode = mode;
  save_watch_face_settings();
}

ClockMode qwatch_settings_get_clock_mode() {
  return g_clock_mode;
}

void qwatch_settings_set_steps_active(bool active) {
  g_steps_active = active;
}

bool qwatch_settings_get_steps_active() {
  return g_steps_active;
}

void qwatch_settings_set_bluetooth_active(bool active) {
  g_bluetooth_active = active;
}

bool qwatch_settings_get_bluetooth_active() {
  return g_bluetooth_active;
}

void qwatch_settings_set_digital_face_index(int index) {
  const int clamped_index = clamp_index(index, watch_faces_get_digital_count());
  if (g_digital_face_index == clamped_index) {
    return;
  }

  g_digital_face_index = clamped_index;
  save_watch_face_settings();
}

int qwatch_settings_get_digital_face_index() {
  return clamp_index(g_digital_face_index, watch_faces_get_digital_count());
}

void qwatch_settings_set_analogue_face_index(int index) {
  const int clamped_index = clamp_index(index, watch_faces_get_analogue_count());
  if (g_analogue_face_index == clamped_index) {
    return;
  }

  g_analogue_face_index = clamped_index;
  save_watch_face_settings();
}

int qwatch_settings_get_analogue_face_index() {
  return clamp_index(g_analogue_face_index, watch_faces_get_analogue_count());
}

void qwatch_settings_set_wifi_ssid(const char* ssid) {
  g_wifi_ssid = ssid ? ssid : "";
}

const char* qwatch_settings_get_wifi_ssid() {
  return g_wifi_ssid.c_str();
}

void qwatch_settings_set_wifi_password(const char* password) {
  g_wifi_password = password ? password : "";
}

const char* qwatch_settings_get_wifi_password() {
  return g_wifi_password.c_str();
}

bool qwatch_settings_save_wifi_credentials() {
  if (!ensure_fs_ready()) {
    Serial.println("[qwatch_settings] Save failed: FFat not ready");
    return false;
  }

  Serial.println("[qwatch_settings] Saving Wi-Fi credentials");
  File f = FFat.open(WIFI_CREDENTIALS_FILE, FILE_WRITE);
  if (!f) {
    Serial.println("[qwatch_settings] Save failed: could not open /wifi_credentials.csv for write");
    return false;
  }

  f.seek(0);
  f.print(g_wifi_ssid);
  f.print(",");
  f.println(g_wifi_password);
  f.close();

  Serial.println("[qwatch_settings] Save complete");
  return true;
}

bool qwatch_settings_load_wifi_credentials() {
  if (!ensure_fs_ready()) {
    Serial.println("[qwatch_settings] Load failed: FFat not ready");
    return false;
  }

  File f = FFat.open(WIFI_CREDENTIALS_FILE, FILE_READ);
  if (!f) {
    Serial.println("[qwatch_settings] Load failed: could not open /wifi_credentials.csv for read");
    return false;
  }

  String line = f.readStringUntil('\n');
  f.close();
  line.trim();

  if (line.length() == 0) {
    Serial.println("[qwatch_settings] Load failed: file empty");
    return false;
  }

  const int comma = line.indexOf(',');
  if (comma < 0) {
    Serial.println("[qwatch_settings] Load failed: no comma separator found");
    return false;
  }

  String ssid = line.substring(0, comma);
  String password = line.substring(comma + 1);

  ssid.trim();
  password.trim();

  g_wifi_ssid = ssid;
  g_wifi_password = password;

  return true;
}

bool qwatch_settings_has_wifi_credentials() {
  return g_wifi_ssid.length() > 0;
}
