#pragma once
#include <stdbool.h>
#include <time.h>

// Call once during setup (non-fatal if WiFi not available).
void time_sync_init();

// Call frequently from loop; handles periodic resync attempts.
void time_sync_loop();

// Get current local time into caller-provided struct tm.
bool time_sync_get_local_time(struct tm *out_tm);

// Format time/date for UI; returns false if not valid.
bool time_sync_format_date(char *out, int out_len);  // "Thu 05 Mar"

// Connection status helpers for UI.
bool time_sync_wifi_is_connected();
bool time_sync_wifi_is_active();
bool time_sync_set_wifi_active(bool active);
bool time_sync_set_bluetooth_active(bool active);
bool time_sync_bluetooth_is_active();

// Battery helpers for UI.
float time_sync_battery_voltage();
int   time_sync_battery_percent();
