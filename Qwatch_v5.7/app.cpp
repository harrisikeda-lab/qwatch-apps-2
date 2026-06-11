#include <Arduino.h>
#include <lvgl.h>

#include "app.h"
#include "display.h"
#include "touch.h"
#include "time_sync.h"
#include "weather_service.h"
#include "app_manager.h"
#include "power_manager.h"
#include "pedometer.h"
#include "qwatch_settings.h"

static uint32_t last_ms = 0;

// Throttle time_sync_loop() so hourly weather / daily NTP scheduling still
// works during fake sleep without pointlessly checking time on every loop pass.
static uint32_t s_last_time_sync_loop_ms = 0;
static constexpr uint32_t TIME_SYNC_START_DELAY_MS = 2000UL;
static constexpr uint32_t TIME_SYNC_LOOP_INTERVAL_MS = 10000UL;

void app_init() {
  Serial.begin(115200);
  delay(200);

  // Load persisted settings before anything consumes them.
  qwatch_settings_init();

  // Weather / location service
  weather_service_init();

  // Real time (NTP + timezone)
  time_sync_init();

  // LVGL core
  lv_init();

  // Hardware drivers
  display_init();
  touch_init();

  // UI / apps
  app_manager_init();

  // Display dim-only handling
  power_manager_init();

  // Step counting
  pedometer_init();

  last_ms = millis();
  s_last_time_sync_loop_ms = 0;
}

void app_loop() {
  power_manager_loop();

  const uint32_t now = millis();

  lv_tick_inc(now - last_ms);
  last_ms = now;

  if (now > TIME_SYNC_START_DELAY_MS &&
      (now - s_last_time_sync_loop_ms) >= TIME_SYNC_LOOP_INTERVAL_MS) {
    s_last_time_sync_loop_ms = now;
    time_sync_loop();
  }

  if (!power_manager_is_sleeping()) {
    weather_service_loop();
  }

  pedometer_loop();

  lv_timer_handler();
  delay(power_manager_is_sleeping() ? 50 : 5);
}
