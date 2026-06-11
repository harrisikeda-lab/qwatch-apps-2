#include "touch.h"

#include <Arduino.h>
#include <lvgl.h>

#include "config.h"
#include "CST816S.h"
#include "app_manager.h"
#include "power_manager.h"
#include "ui_settings.h"

// CST816S (I2C touch controller)
static CST816S touch(TOUCH_I2C_SDA, TOUCH_I2C_SCL, TOUCH_RST, TOUCH_IRQ);

// LVGL input device
static lv_indev_t* indev_touch = nullptr;

// Persistent state
static bool g_pressed = false;
static int16_t g_x = 120;
static int16_t g_y = 120;

// Software swipe tracking
static bool g_touch_active = false;
static bool g_swipe_fired = false;
static int16_t g_start_x = 120;
static int16_t g_start_y = 120;
static uint32_t g_touch_start_ms = 0;
static uint32_t g_last_touch_ms = 0;

// After a gesture-driven screen change, suppress taps briefly
static uint32_t g_suppress_press_until = 0;

// Tunables
static constexpr uint32_t TOUCH_STUCK_TIMEOUT_MS = 250;
static constexpr uint32_t PRESS_SUPPRESS_MS = 180;
static constexpr int16_t SWIPE_THRESHOLD_PX = 44;
static constexpr int16_t AXIS_DOMINANCE_PX = 12;
static constexpr uint32_t MAX_SWIPE_TIME_MS = 650;

static void clamp_xy() {
  if (g_x < 0) g_x = 0;
  if (g_y < 0) g_y = 0;
  if (g_x > 239) g_x = 239;
  if (g_y > 239) g_y = 239;
}

static int16_t abs16(int16_t v) {
  return v < 0 ? -v : v;
}

static void handle_gesture(uint8_t gid) {
  switch (gid) {
    case SWIPE_UP:
      if (!ui_settings_handle_swipe_up()) {
        app_manager_on_swipe_up();
      }
      break;
    case SWIPE_DOWN:
      if (!ui_settings_handle_swipe_down()) {
        app_manager_on_swipe_down();
      }
      break;
    case SWIPE_LEFT:
      if (!ui_settings_handle_swipe_left()) {
        app_manager_on_swipe_left();
      }
      break;
    case SWIPE_RIGHT:
      if (!ui_settings_handle_swipe_right()) {
        app_manager_on_swipe_right();
      }
      break;
    default:
      break;
  }
}

static void begin_touch_cycle(uint32_t now) {
  g_touch_active = true;
  g_swipe_fired = false;
  g_start_x = g_x;
  g_start_y = g_y;
  g_touch_start_ms = now;
  g_last_touch_ms = now;
}

static void end_touch_cycle() {
  g_touch_active = false;
  g_swipe_fired = false;
}

static void maybe_fire_swipe(uint32_t now) {
  if (!g_touch_active || g_swipe_fired) {
    return;
  }

  // Keep raw touch coordinates for LVGL button hit testing.
  // Rotate only the gesture vector to match a 90 degree rotated screen.
  const int16_t raw_dx = g_x - g_start_x;
  const int16_t raw_dy = g_y - g_start_y;

  const int16_t dx = -raw_dy;
  const int16_t dy = raw_dx;

  const int16_t adx = abs16(dx);
  const int16_t ady = abs16(dy);

  if (now - g_touch_start_ms > MAX_SWIPE_TIME_MS) {
    return;
  }

  uint8_t gid = NONE;

  if (adx >= SWIPE_THRESHOLD_PX && adx >= (ady + AXIS_DOMINANCE_PX)) {
    gid = (dx > 0) ? SWIPE_RIGHT : SWIPE_LEFT;
  } else if (ady >= SWIPE_THRESHOLD_PX && ady >= (adx + AXIS_DOMINANCE_PX)) {
    gid = (dy > 0) ? SWIPE_DOWN : SWIPE_UP;
  }

  if (gid == NONE) {
    return;
  }

  g_swipe_fired = true;
  g_pressed = false;
  g_suppress_press_until = now + PRESS_SUPPRESS_MS;
  handle_gesture(gid);
}

static void lv_touch_read(lv_indev_drv_t* drv, lv_indev_data_t* data) {
  (void)drv;

  const uint32_t now = millis();
  bool saw_sample = false;
  bool saw_release = false;

  if (touch.available()) {
    saw_sample = true;

    const int16_t raw_x = (int16_t)touch.data.x;
    const int16_t raw_y = (int16_t)touch.data.y;

    // Pass raw physical coordinates to LVGL.
    // With LVGL display rotation enabled, LVGL handles pointer rotation.
    g_x = raw_x;
    g_y = raw_y;
    clamp_xy();

    // CST816S events: 0 = Down, 1 = Up, 2 = Contact
    if (touch.data.event == 1 || touch.data.points == 0) {
      power_manager_notify_activity();
      g_pressed = false;
      saw_release = true;
    } else {
      if (!g_touch_active) {
        begin_touch_cycle(now);
      }

      power_manager_notify_activity();
      g_last_touch_ms = now;
      g_pressed = true;
      maybe_fire_swipe(now);
    }
  }

  // If the controller/IRQ misses a clean release, do not stay stuck pressed.
  if (g_touch_active && !saw_sample && (now - g_last_touch_ms > TOUCH_STUCK_TIMEOUT_MS)) {
    g_pressed = false;
    saw_release = true;
  }

  if (saw_release) {
    end_touch_cycle();
  }

  if (now < g_suppress_press_until) {
    g_pressed = false;
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = g_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
  }

  data->point.x = g_x;
  data->point.y = g_y;
}

void touch_init() {
  // RISING performed better than FALLING on this board variant in testing.
  // The driver below now also polls the IRQ line / active-touch state so it
  // is much less dependent on a single perfect interrupt edge.
  touch.begin(RISING);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lv_touch_read;

  indev_touch = lv_indev_drv_register(&indev_drv);
  (void)indev_touch;
}
