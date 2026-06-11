#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  CLOCK_MODE_DIGITAL = 0,
  CLOCK_MODE_ANALOGUE = 1
} ClockMode;

void qwatch_settings_init();

void qwatch_settings_set_clock_mode(ClockMode mode);
ClockMode qwatch_settings_get_clock_mode();

void qwatch_settings_set_steps_active(bool active);
bool qwatch_settings_get_steps_active();

void qwatch_settings_set_bluetooth_active(bool active);
bool qwatch_settings_get_bluetooth_active();

void qwatch_settings_set_digital_face_index(int index);
int qwatch_settings_get_digital_face_index();

void qwatch_settings_set_analogue_face_index(int index);
int qwatch_settings_get_analogue_face_index();

void qwatch_settings_set_wifi_ssid(const char* ssid);
const char* qwatch_settings_get_wifi_ssid();

void qwatch_settings_set_wifi_password(const char* password);
const char* qwatch_settings_get_wifi_password();

bool qwatch_settings_save_wifi_credentials();
bool qwatch_settings_load_wifi_credentials();
bool qwatch_settings_has_wifi_credentials();

#ifdef __cplusplus
}
#endif
