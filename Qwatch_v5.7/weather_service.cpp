#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "weather_service.h"

static const uint32_t WEATHER_REFRESH_INTERVAL_MS = 60UL * 60UL * 1000UL;
static const uint32_t RETRY_INTERVAL_MS           = 10UL * 60UL * 1000UL;

static bool     s_weather_valid       = false;
static bool     s_have_location       = false;
static int      s_temperature_c       = 0;
static int      s_weather_code        = -1;
static int32_t  s_utc_offset_seconds  = 0;
static float    s_latitude            = 0.0f;
static float    s_longitude           = 0.0f;
static uint32_t s_next_refresh_ms     = 0;

static bool http_get_plain(const String &url, String &response) {
  HTTPClient http;
  if (!http.begin(url)) {
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  response = http.getString();
  http.end();
  return true;
}

static bool http_get_secure(const String &url, String &response) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  if (!http.begin(client, url)) {
    return false;
  }

  const int code = http.GET();
  if (code != HTTP_CODE_OK) {
    http.end();
    return false;
  }

  response = http.getString();
  http.end();
  return true;
}

static bool fetch_ip_location() {
  String response;
  if (!http_get_plain(
        "http://ip-api.com/json/?fields=status,message,city,lat,lon",
        response)) {
    return false;
  }

  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    return false;
  }

  const char *status = doc["status"] | "";
  if (strcmp(status, "success") != 0) {
    return false;
  }

  const char *city = doc["city"] | "";
  if (city[0] == '\0') {
    return false;
  }

  if (!doc["lat"].is<float>() || !doc["lon"].is<float>()) {
    return false;
  }

  s_latitude = doc["lat"].as<float>();
  s_longitude = doc["lon"].as<float>();
  s_have_location = true;
  return true;
}

static bool fetch_current_weather() {
  if (!s_have_location) {
    return false;
  }

  String url = "https://api.open-meteo.com/v1/forecast?latitude=" + String(s_latitude, 4) +
               "&longitude=" + String(s_longitude, 4) +
               "&current=temperature_2m,weather_code&temperature_unit=celsius&timezone=auto";

  String response;
  if (!http_get_secure(url, response)) {
    return false;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    return false;
  }

  JsonVariant current = doc["current"];
  if (current.isNull()) {
    return false;
  }

  if (!current["temperature_2m"].is<float>() ||
      !current["weather_code"].is<int>() ||
      !doc["utc_offset_seconds"].is<int>()) {
    return false;
  }

  const float temp = current["temperature_2m"].as<float>();
  const int code = current["weather_code"].as<int>();
  const int32_t utc_offset_seconds = doc["utc_offset_seconds"].as<int32_t>();

  s_temperature_c = (int)(temp >= 0.0f ? temp + 0.5f : temp - 0.5f);
  s_weather_code = code;
  s_utc_offset_seconds = utc_offset_seconds;
  s_weather_valid = true;

  Serial.print("[weather] Timezone: ");
  Serial.println(doc["timezone"] | "unknown");
  Serial.print("[weather] UTC offset seconds: ");
  Serial.println(s_utc_offset_seconds);
  return true;
}

static const char *map_weather_icon(int code) {
  switch (code) {
    case 0: return "\uf00d"; // day-sunny
    case 1: return "\uf002"; // day-cloudy
    case 2: return "\uf002"; // day-cloudy
    case 3: return "\uf013"; // cloudy

    case 45:
    case 48: return "\uf014"; // fog

    case 51:
    case 53:
    case 55: return "\uf01c"; // sprinkle

    case 56:
    case 57: return "\uf0b5"; // sleet

    case 61:
    case 63:
    case 65: return "\uf019"; // rain

    case 66:
    case 67: return "\uf017"; // rain-mix

    case 71:
    case 73:
    case 75:
    case 77: return "\uf01b"; // snow

    case 80:
    case 81:
    case 82: return "\uf01a"; // showers

    case 85:
    case 86: return "\uf01b"; // snow

    case 95: return "\uf01e"; // thunderstorm

    case 96:
    case 99: return "\uf01d"; // storm-showers

    default: return "\uf07b"; // na
  }
}

static void refresh_weather_if_due() {
  const uint32_t now = millis();
  if ((int32_t)(now - s_next_refresh_ms) < 0) {
    return;
  }

  weather_service_refresh_now();
}

void weather_service_init() {
  s_weather_valid = false;
  s_have_location = false;
  s_temperature_c = 0;
  s_weather_code = -1;
  s_utc_offset_seconds = 0;
  s_latitude = 0.0f;
  s_longitude = 0.0f;
  s_next_refresh_ms = 0;
}

void weather_service_loop() {
  refresh_weather_if_due();
}

bool weather_service_refresh_now() {
  const uint32_t now = millis();

  if (WiFi.status() != WL_CONNECTED) {
    s_next_refresh_ms = now + RETRY_INTERVAL_MS;
    return false;
  }

  bool ok = true;

  if (!s_have_location) {
    ok = fetch_ip_location();
  }

  if (ok) {
    ok = fetch_current_weather();
  }

  // IMPORTANT:
  // If refresh fails, keep the previous valid weather data and icon.
  // Only adjust the retry timing. Do NOT clear s_weather_valid here.
  s_next_refresh_ms = now + (ok ? WEATHER_REFRESH_INTERVAL_MS : RETRY_INTERVAL_MS);
  return ok;
}

bool weather_service_is_valid() {
  return s_weather_valid;
}

int weather_service_get_temperature_c() {
  return s_temperature_c;
}

int32_t weather_service_get_utc_offset_seconds() {
  return s_utc_offset_seconds;
}

const char* weather_service_get_icon_utf8() {
  return map_weather_icon(s_weather_code);
}
