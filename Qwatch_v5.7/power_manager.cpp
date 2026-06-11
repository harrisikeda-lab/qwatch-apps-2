#include "power_manager.h"

#include <Arduino.h>

#include "display.h"
#include "QMI8658.h"
#include "ui_watch.h"

namespace {

constexpr uint32_t DIM_AFTER_MS = 5000;
constexpr uint32_t SCREEN_OFF_AFTER_MS = 10000;


bool g_dimmed = false;
bool g_sleeping = false;
bool g_screen_off = false;
bool g_imu_ready = false;
uint32_t g_last_activity_ms = 0;

void imu_apply_active_config() {
  QMI8658Config cfg{};
  cfg.inputSelection = QMI8658_CONFIG_ACCGYR_ENABLE;
  cfg.accRange = QMI8658AccRange_8g;
  cfg.accOdr = QMI8658AccOdr_125Hz;
  cfg.gyrRange = QMI8658GyrRange_512dps;
  cfg.gyrOdr = QMI8658GyrOdr_125Hz;
  cfg.magDev = MagDev_AKM09918;
  cfg.magOdr = QMI8658MagOdr_125Hz;
  cfg.aeOdr = QMI8658AeOdr_128Hz;
  QMI8658_Config_apply(&cfg);
}

}  // namespace

void power_manager_init() {
  g_last_activity_ms = millis();
  g_dimmed = false;
  g_sleeping = false;
  g_screen_off = false;

  pinMode(IMU_INT1, INPUT_PULLUP);

  const int imu_init_result = QMI8658_init();
  g_imu_ready = (imu_init_result == 1);

  if (g_imu_ready) {
    imu_apply_active_config();
  }

  display_set_dimmed(false);
  display_set_sleep(false);
}

void power_manager_notify_activity() {
  g_last_activity_ms = millis();

  if (g_screen_off) {
    g_screen_off = false;
    setCpuFrequencyMhz(160);
    ui_watch_refresh_now();
    display_set_sleep(false);
  }

  if (g_dimmed) {
    g_dimmed = false;
    display_set_dimmed(false);
  }
}

bool power_manager_loop() {
  const uint32_t now = millis();
  const uint32_t idle_ms = now - g_last_activity_ms;

  if (!g_dimmed && idle_ms >= DIM_AFTER_MS) {
    g_dimmed = true;
    display_set_dimmed(true);
  }

  if (!g_screen_off && idle_ms >= SCREEN_OFF_AFTER_MS) {
    g_screen_off = true;
    setCpuFrequencyMhz(80);
    display_set_sleep(true);
  }

  return false;
}
bool power_manager_is_sleeping() {
  return g_sleeping;
}
