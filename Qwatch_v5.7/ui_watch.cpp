#include <Arduino.h>
#include <lvgl.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include "ui_watch.h"
#include "fonts.h"
#include "images.h"
#include "qwatch_settings.h"
#include "time_sync.h"
#include "watch_faces.h"
#include "weather_service.h"
#include "display.h"

namespace {

constexpr int WATCH_SIZE = 240;
constexpr int CENTER_X = WATCH_SIZE / 2;
constexpr int CENTER_Y = WATCH_SIZE / 2;
constexpr float DEG_TO_RAD_F = 0.017453292519943295769f;
constexpr int ANALOGUE_DATE_X = 0;

enum DigitalDateMode : uint8_t {
  DIGITAL_DATE_FORMATTED = 0,
  DIGITAL_DATE_DAY_MDAY = 1
};

static lv_obj_t *watch_screen;
static lv_obj_t *digital_panel;
static lv_obj_t *analogue_panel;

static lv_obj_t *label_time;
static lv_obj_t *label_hour;
static lv_obj_t *label_minute;
static lv_obj_t *label_date;
static lv_obj_t *label_wifi;
static lv_obj_t *label_bt;
static lv_obj_t *label_battery;
static lv_obj_t *label_weather_icon;
static lv_obj_t *label_weather_temp;
static lv_obj_t *chronos_date_box;

static lv_obj_t *chronos_seconds_ring;
static lv_obj_t *chronos_minutes_ring;
static lv_obj_t *chronos_hours_ring;

static lv_obj_t *digital_bg;
static lv_obj_t *analogue_bg;
static lv_obj_t *analogue_date_label;
static lv_obj_t *hour_hand;
static lv_obj_t *minute_hand;
static lv_obj_t *second_hand;
static lv_obj_t *centre_dot;

static lv_point_t hour_points[2];
static lv_point_t minute_points[2];
static lv_point_t second_points[2];

static ClockMode g_applied_mode = (ClockMode)-1;
static int g_applied_digital_face = -1;
static int g_applied_analogue_face = -1;

static int32_t g_hour_angle10 = 0;
static int32_t g_minute_angle10 = 0;
static int32_t g_second_angle10 = 0;
static bool g_analogue_angles_initialised = false;

static void update_radio_icons() {
  const lv_color_t connected_col = lv_color_make(0, 255, 0);
  const lv_color_t bluetooth_active_col = lv_color_make(80, 160, 255);
  const lv_color_t disconnected_col = lv_color_white();

  lv_obj_set_style_text_color(
    label_wifi,
    time_sync_wifi_is_connected() ? connected_col : disconnected_col,
    0
  );

  lv_obj_set_style_text_color(
    label_bt,
    time_sync_bluetooth_is_active() ? bluetooth_active_col : disconnected_col,
    0
  );
}

static void update_battery_icon() {
  const int pct = time_sync_battery_percent();
  const lv_color_t healthy_col = lv_color_make(0, 255, 0);
  const lv_color_t low_col = lv_color_white();

  const char *icon = LV_SYMBOL_BATTERY_EMPTY;

  if (pct >= 20) {
    icon = LV_SYMBOL_BATTERY_FULL;
  } else if (pct >= 10) {
    icon = LV_SYMBOL_BATTERY_3;
  } else if (pct >= 5) {
    icon = LV_SYMBOL_BATTERY_2;
  } else if (pct >= 2) {
    icon = LV_SYMBOL_BATTERY_1;
  }

  lv_label_set_text(label_battery, icon);
  lv_obj_set_style_text_color(label_battery, pct >= 20 ? healthy_col : low_col, 0);
}

static void update_weather() {
  if (!weather_service_is_valid()) {
    lv_label_set_text(label_weather_icon, "\uf07b");
    lv_label_set_text(label_weather_temp, "--\xC2\xB0");
    return;
  }

  lv_label_set_text(label_weather_icon, weather_service_get_icon_utf8());

  char tbuf[16];
  snprintf(tbuf, sizeof(tbuf), "%d\xC2\xB0", weather_service_get_temperature_c());
  lv_label_set_text(label_weather_temp, tbuf);
}

static void update_digital_date_box(const DigitalWatchFaceDef* face) {
  if (!chronos_date_box || !label_date) {
    return;
  }

  if (!face || !face->show_date_box || !face->show_date) {
    lv_obj_add_flag(chronos_date_box, LV_OBJ_FLAG_HIDDEN);
    return;
  }

  lv_obj_update_layout(label_date);

  const lv_coord_t text_w = lv_obj_get_width(label_date);
  const lv_coord_t text_h = lv_obj_get_height(label_date);

  constexpr lv_coord_t pad_x = 12;
  constexpr lv_coord_t pad_y = 6;

  lv_obj_set_size(chronos_date_box, text_w + (pad_x * 2), text_h + (pad_y * 2));
  lv_obj_align(chronos_date_box, face->date_align, face->date_x, face->date_y - 6);
  lv_obj_align_to(label_date, chronos_date_box, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(chronos_date_box, LV_OBJ_FLAG_HIDDEN);
}

static void style_chronos_ring(
  lv_obj_t *ring,
  int diameter,
  int center_x,
  int center_y,
  int thickness,
  int range_max,
  lv_color_t bg_color,
  lv_color_t indicator_color
) {
  lv_obj_set_size(ring, diameter, diameter);
  lv_obj_set_pos(ring, center_x - (diameter / 2), center_y - (diameter / 2));
  lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);

  lv_arc_set_rotation(ring, 270);
  lv_arc_set_bg_angles(ring, 0, 360);
  lv_arc_set_range(ring, 0, range_max);
  lv_arc_set_value(ring, 0);

  lv_obj_set_style_arc_width(ring, thickness, LV_PART_MAIN);
  lv_obj_set_style_arc_color(ring, bg_color, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(ring, false, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(ring, LV_OPA_COVER, LV_PART_MAIN);

  lv_obj_set_style_arc_width(ring, thickness, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(ring, indicator_color, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(ring, false, LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(ring, LV_OPA_COVER, LV_PART_INDICATOR);

  lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, LV_PART_KNOB);
  lv_obj_set_style_border_opa(ring, LV_PART_KNOB, LV_OPA_TRANSP);
  lv_obj_set_style_pad_all(ring, 0, LV_PART_KNOB);

  lv_obj_add_flag(ring, LV_OBJ_FLAG_HIDDEN);
}

static void setup_chronos_seconds_ring() {
  constexpr int center_x = 182;
  constexpr int center_y = 163;
  constexpr int ring_thickness = 5;
  constexpr int ring_gap = 2;

  constexpr int seconds_outer_radius = 34;
  constexpr int minutes_outer_radius = seconds_outer_radius - ring_thickness - ring_gap;
  constexpr int hours_outer_radius = minutes_outer_radius - ring_thickness - ring_gap;

  const lv_color_t bg_grey = lv_color_make(110, 110, 110);
  const lv_color_t sec_red = lv_color_make(255, 0, 0);
  const lv_color_t min_green = lv_color_make(0, 255, 0);
  const lv_color_t hr_blue = lv_color_make(0, 120, 255);

  chronos_seconds_ring = lv_arc_create(digital_panel);
  chronos_minutes_ring = lv_arc_create(digital_panel);
  chronos_hours_ring = lv_arc_create(digital_panel);

  style_chronos_ring(
    chronos_seconds_ring,
    seconds_outer_radius * 2,
    center_x,
    center_y,
    ring_thickness,
    60,
    bg_grey,
    sec_red
  );

  style_chronos_ring(
    chronos_minutes_ring,
    minutes_outer_radius * 2,
    center_x,
    center_y,
    ring_thickness,
    60,
    bg_grey,
    min_green
  );

  style_chronos_ring(
    chronos_hours_ring,
    hours_outer_radius * 2,
    center_x,
    center_y,
    ring_thickness,
    24,
    bg_grey,
    hr_blue
  );
}

static void set_chronos_seconds_ring_visible(bool visible) {
  if (chronos_seconds_ring) {
    if (visible) lv_obj_clear_flag(chronos_seconds_ring, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(chronos_seconds_ring, LV_OBJ_FLAG_HIDDEN);
  }

  if (chronos_minutes_ring) {
    if (visible) lv_obj_clear_flag(chronos_minutes_ring, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(chronos_minutes_ring, LV_OBJ_FLAG_HIDDEN);
  }

  if (chronos_hours_ring) {
    if (visible) lv_obj_clear_flag(chronos_hours_ring, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(chronos_hours_ring, LV_OBJ_FLAG_HIDDEN);
  }
}

static void update_chronos_seconds_ring(bool enabled, int hour_value, int minute_value, int seconds_value) {
  set_chronos_seconds_ring_visible(enabled);

  if (!chronos_seconds_ring || !chronos_minutes_ring || !chronos_hours_ring) {
    return;
  }

  if (!enabled) {
    lv_arc_set_value(chronos_seconds_ring, 0);
    lv_arc_set_value(chronos_minutes_ring, 0);
    lv_arc_set_value(chronos_hours_ring, 0);
    return;
  }

  lv_arc_set_value(chronos_seconds_ring, seconds_value);
  lv_arc_set_value(chronos_minutes_ring, minute_value);
  lv_arc_set_value(chronos_hours_ring, hour_value % 24);
}

static const AnalogueWatchFaceDef* current_analogue_face() {
  return watch_faces_get_analogue(qwatch_settings_get_analogue_face_index());
}

static void set_hand_points(lv_obj_t* line, lv_point_t pts[2], float angle_deg, int length) {
  const float rad = (angle_deg - 90.0f) * DEG_TO_RAD_F;
  pts[0].x = CENTER_X;
  pts[0].y = CENTER_Y;
  pts[1].x = (lv_coord_t)lroundf((float)CENTER_X + cosf(rad) * (float)length);
  pts[1].y = (lv_coord_t)lroundf((float)CENTER_Y + sinf(rad) * (float)length);
  lv_line_set_points(line, pts, 2);
}

static void set_hour_angle10(int32_t angle10) {
  const AnalogueWatchFaceDef* face = current_analogue_face();
  g_hour_angle10 = angle10;
  set_hand_points(hour_hand, hour_points, (float)(angle10 % 3600) / 10.0f, face ? face->hour_length : 46);
}

static void set_minute_angle10(int32_t angle10) {
  const AnalogueWatchFaceDef* face = current_analogue_face();
  g_minute_angle10 = angle10;
  set_hand_points(minute_hand, minute_points, (float)(angle10 % 3600) / 10.0f, face ? face->minute_length : 66);
}

static void set_second_angle10(int32_t angle10) {
  const AnalogueWatchFaceDef* face = current_analogue_face();
  g_second_angle10 = angle10;
  set_hand_points(second_hand, second_points, (float)(angle10 % 3600) / 10.0f, face ? face->second_length : 78);
}

static void hour_anim_exec_cb(void* var, int32_t value) {
  (void)var;
  set_hour_angle10(value);
}

static void minute_anim_exec_cb(void* var, int32_t value) {
  (void)var;
  set_minute_angle10(value);
}

static void second_anim_exec_cb(void* var, int32_t value) {
  (void)var;
  set_second_angle10(value);
}

static int32_t unwrap_forward_angle10(int32_t current_angle10, int32_t target_angle10) {
  while (target_angle10 < current_angle10) {
    target_angle10 += 3600;
  }
  return target_angle10;
}

static void stop_hand_animations() {
  if (hour_hand) lv_anim_del(hour_hand, hour_anim_exec_cb);
  if (minute_hand) lv_anim_del(minute_hand, minute_anim_exec_cb);
  if (second_hand) lv_anim_del(second_hand, second_anim_exec_cb);
}

static void animate_hand_to(lv_obj_t* hand, lv_anim_exec_xcb_t exec_cb, int32_t from, int32_t to, uint32_t duration) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, hand);
  lv_anim_set_exec_cb(&a, exec_cb);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_time(&a, duration);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  lv_anim_start(&a);
}

static bool analogue_watch_should_run() {
  return (qwatch_settings_get_clock_mode() == CLOCK_MODE_ANALOGUE) &&
         watch_screen &&
         (lv_scr_act() == watch_screen);
}

static void sync_watch_mode() {
  const bool digital = (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL);

  if (digital_panel) {
    if (digital) lv_obj_clear_flag(digital_panel, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(digital_panel, LV_OBJ_FLAG_HIDDEN);
  }

  if (analogue_panel) {
    if (digital) lv_obj_add_flag(analogue_panel, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_clear_flag(analogue_panel, LV_OBJ_FLAG_HIDDEN);
  }
}

static lv_obj_t* create_panel(lv_obj_t* parent) {
  lv_obj_t* panel = lv_obj_create(parent);
  lv_obj_set_size(panel, WATCH_SIZE, WATCH_SIZE);
  lv_obj_center(panel);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(panel, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(panel, 0, 0);
  lv_obj_set_style_radius(panel, 0, 0);
  lv_obj_set_style_pad_all(panel, 0, 0);
  return panel;
}

static void style_hand(lv_obj_t* hand, uint16_t width, lv_color_t color, bool rounded) {
  lv_obj_set_style_line_width(hand, width, 0);
  lv_obj_set_style_line_color(hand, color, 0);
  lv_obj_set_style_line_rounded(hand, rounded, 0);
  lv_obj_set_style_line_opa(hand, LV_OPA_COVER, 0);
}

static void apply_digital_face(int index) {
  const DigitalWatchFaceDef* face = watch_faces_get_digital(index);
  if (!face) return;

  lv_img_set_src(digital_bg, face->image);
  lv_img_set_zoom(digital_bg, face->main_zoom);
  lv_obj_center(digital_bg);

  if (face->time_font) {
    lv_obj_set_style_text_font(label_time, face->time_font, 0);
    lv_obj_set_style_text_font(label_hour, face->time_font, 0);
    lv_obj_set_style_text_font(label_minute, face->time_font, 0);
  }

  if (face->date_font) {
    lv_obj_set_style_text_font(label_date, face->date_font, 0);
  }

  lv_obj_set_style_text_color(label_time, face->time_color, 0);
  lv_obj_set_style_text_color(label_hour, face->time_color, 0);
  lv_obj_set_style_text_color(label_minute, face->time_color, 0);
  lv_obj_set_style_text_color(label_date, face->date_color, 0);
  lv_obj_set_style_text_align(label_date, face->date_text_align, 0);

  if (face->split_time) {
    lv_obj_add_flag(label_time, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(label_hour, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(label_minute, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_hour, face->hour_align, face->hour_x, face->hour_y);
    lv_obj_align(label_minute, face->minute_align, face->minute_x, face->minute_y);
  } else {
    lv_obj_clear_flag(label_time, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(label_hour, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(label_minute, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_time, face->time_align, face->time_x, face->time_y);
  }

  if (face->show_date) {
    lv_obj_clear_flag(label_date, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_date, face->date_align, face->date_x, face->date_y);
  } else {
    lv_obj_add_flag(label_date, LV_OBJ_FLAG_HIDDEN);
  }

  if (face->show_weather_icon) {
    lv_obj_clear_flag(label_weather_icon, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_weather_icon, face->weather_icon_align, face->weather_icon_x, face->weather_icon_y);
  } else {
    lv_obj_add_flag(label_weather_icon, LV_OBJ_FLAG_HIDDEN);
  }

  if (face->show_weather_temp) {
    lv_obj_clear_flag(label_weather_temp, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_weather_temp, face->weather_temp_align, face->weather_temp_x, face->weather_temp_y);
  } else {
    lv_obj_add_flag(label_weather_temp, LV_OBJ_FLAG_HIDDEN);
  }

  if (face->show_battery) {
    lv_obj_clear_flag(label_battery, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_battery, face->battery_align, face->battery_x, face->battery_y);
  } else {
    lv_obj_add_flag(label_battery, LV_OBJ_FLAG_HIDDEN);
  }

  if (face->show_wifi) {
    lv_obj_clear_flag(label_wifi, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_wifi, face->wifi_align, face->wifi_x, face->wifi_y);
  } else {
    lv_obj_add_flag(label_wifi, LV_OBJ_FLAG_HIDDEN);
  }

  if (face->show_bt) {
    lv_obj_clear_flag(label_bt, LV_OBJ_FLAG_HIDDEN);
    lv_obj_align(label_bt, face->bt_align, face->bt_x, face->bt_y);
  } else {
    lv_obj_add_flag(label_bt, LV_OBJ_FLAG_HIDDEN);
  }

  update_digital_date_box(face);
  set_chronos_seconds_ring_visible(face->show_rings != 0);
}

static void apply_analogue_face(int index) {
  const AnalogueWatchFaceDef* face = watch_faces_get_analogue(index);
  if (!face) return;

  stop_hand_animations();

  lv_img_set_src(analogue_bg, face->image);
  lv_obj_center(analogue_bg);

  style_hand(hour_hand, face->hour_width, face->hour_color, face->line_rounded != 0);
  style_hand(minute_hand, face->minute_width, face->minute_color, face->line_rounded != 0);
  style_hand(second_hand, face->second_width, face->second_color, face->line_rounded != 0);

  lv_obj_set_size(centre_dot, face->centre_dot_size, face->centre_dot_size);
  lv_obj_set_style_bg_color(centre_dot, face->centre_dot_color, 0);
  lv_obj_align(centre_dot, LV_ALIGN_CENTER, 0, 0);

  if (face->show_date) {
    lv_obj_clear_flag(analogue_date_label, LV_OBJ_FLAG_HIDDEN);

    if (face->image == &brix) {
      lv_obj_set_style_text_font(analogue_date_label, &lv_font_montserrat_18, 0);
    } else {
      lv_obj_set_style_text_font(analogue_date_label, &lv_font_montserrat_14, 0);
    }

    lv_obj_align(analogue_date_label, LV_ALIGN_CENTER, ANALOGUE_DATE_X, face->date_y);
  } else {
    lv_obj_add_flag(analogue_date_label, LV_OBJ_FLAG_HIDDEN);
  }

  g_analogue_angles_initialised = false;
}

static void sync_face_configuration() {
  const ClockMode mode = qwatch_settings_get_clock_mode();
  const int digital_face = qwatch_settings_get_digital_face_index();
  const int analogue_face = qwatch_settings_get_analogue_face_index();

  if (digital_face != g_applied_digital_face) {
    apply_digital_face(digital_face);
    g_applied_digital_face = digital_face;
  }

  if (analogue_face != g_applied_analogue_face) {
    apply_analogue_face(analogue_face);
    g_applied_analogue_face = analogue_face;
  }

  if (mode != g_applied_mode) {
    sync_watch_mode();
    g_applied_mode = mode;
  }
}

static bool get_current_time_tm(struct tm* out_tm, struct timeval* out_tv = nullptr) {
  if (!out_tm) return false;

  struct timeval tv;
  if (gettimeofday(&tv, NULL) != 0) {
    return false;
  }

  time_t now_sec = tv.tv_sec;
  if (now_sec < 1672531200) {
    return false;
  }

  localtime_r(&now_sec, out_tm);
  if (out_tv) {
    *out_tv = tv;
  }
  return true;
}

static void update_analogue_date() {
  const AnalogueWatchFaceDef* face = current_analogue_face();
  if (!face || !face->show_date || !analogue_date_label) {
    return;
  }

  struct tm now_tm;
  if (!get_current_time_tm(&now_tm)) {
    lv_label_set_text(analogue_date_label, "--");
    return;
  }

  if (face->image == &brix) {
    static const char* const day_names[] = {
      "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
    };
    char dbuf[16];
    snprintf(dbuf, sizeof(dbuf), "%s %02d", day_names[now_tm.tm_wday], now_tm.tm_mday);
    lv_label_set_text(analogue_date_label, dbuf);
  } else {
    char dbuf[8];
    snprintf(dbuf, sizeof(dbuf), "%02d", now_tm.tm_mday);
    lv_label_set_text(analogue_date_label, dbuf);
  }
}

static void update_analogue_hands() {
  const AnalogueWatchFaceDef* face = current_analogue_face();
  const int hour_length = face ? face->hour_length : 46;
  const int minute_length = face ? face->minute_length : 66;
  const int second_length = face ? face->second_length : 78;

  struct timeval tv;
  struct tm now_tm;
  if (!get_current_time_tm(&now_tm, &tv)) {
    stop_hand_animations();
    g_analogue_angles_initialised = false;
    set_hand_points(hour_hand, hour_points, 0.0f, hour_length);
    set_hand_points(minute_hand, minute_points, 0.0f, minute_length);
    set_hand_points(second_hand, second_points, 0.0f, second_length);
    return;
  }

  const float fractional_seconds = (float)tv.tv_usec / 1000000.0f;
  const float seconds = (float)now_tm.tm_sec + fractional_seconds;
  const float minutes = (float)now_tm.tm_min + (seconds / 60.0f);
  const float hours = (float)(now_tm.tm_hour % 12) + (minutes / 60.0f);

  const int32_t target_second_angle10 = (int32_t)lroundf(seconds * 60.0f);
  const int32_t target_minute_angle10 = (int32_t)lroundf(minutes * 60.0f);
  const int32_t target_hour_angle10 = (int32_t)lroundf(hours * 300.0f);

  if (!face || !face->animate_hands) {
    stop_hand_animations();
    g_analogue_angles_initialised = true;
    set_hour_angle10(target_hour_angle10);
    set_minute_angle10(target_minute_angle10);
    set_second_angle10(target_second_angle10);
    return;
  }

  if (!g_analogue_angles_initialised) {
    g_analogue_angles_initialised = true;
    set_hour_angle10(target_hour_angle10);
    set_minute_angle10(target_minute_angle10);
    set_second_angle10(target_second_angle10);
    return;
  }

  const int32_t hour_now = g_hour_angle10 % 3600;
  const int32_t minute_now = g_minute_angle10 % 3600;
  const int32_t second_now = g_second_angle10 % 3600;

  if (abs(target_hour_angle10 - hour_now) > 300 ||
      abs(target_minute_angle10 - minute_now) > 900 ||
      abs(target_second_angle10 - second_now) > 1200) {
    stop_hand_animations();
    set_hour_angle10(target_hour_angle10);
    set_minute_angle10(target_minute_angle10);
    set_second_angle10(target_second_angle10);
    return;
  }

  stop_hand_animations();
  animate_hand_to(hour_hand, hour_anim_exec_cb, g_hour_angle10, unwrap_forward_angle10(g_hour_angle10, target_hour_angle10), 850);
  animate_hand_to(minute_hand, minute_anim_exec_cb, g_minute_angle10, unwrap_forward_angle10(g_minute_angle10, target_minute_angle10), 850);
  animate_hand_to(second_hand, second_anim_exec_cb, g_second_angle10, unwrap_forward_angle10(g_second_angle10, target_second_angle10), 850);
}

static void update_watch_ui(bool allow_when_sleeping) {
  if (display_is_sleeping() && !allow_when_sleeping) {
    stop_hand_animations();
    return;
  }

  char tbuf[16];
  char dbuf[24];
  struct tm now_tm;
  const bool have_time = time_sync_get_local_time(&now_tm);
  const DigitalWatchFaceDef* digital_face = watch_faces_get_digital(qwatch_settings_get_digital_face_index());

  if (have_time) {
    if (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL &&
        digital_face &&
        !digital_face->split_time &&
        !digital_face->show_seconds) {
      snprintf(tbuf, sizeof(tbuf), "%02d:%02d", now_tm.tm_hour, now_tm.tm_min);
    } else {
      snprintf(tbuf, sizeof(tbuf), "%02d:%02d:%02d", now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec);
    }
    lv_label_set_text(label_time, tbuf);

    char hbuf[8];
    char mbuf[8];
    snprintf(hbuf, sizeof(hbuf), "%02d", now_tm.tm_hour);
    snprintf(mbuf, sizeof(mbuf), "%02d", now_tm.tm_min);
    lv_label_set_text(label_hour, hbuf);
    lv_label_set_text(label_minute, mbuf);

    if (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL && digital_face && digital_face->date_mode == DIGITAL_DATE_DAY_MDAY) {
      static const char* const day_names[] = {
        "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
      };
      snprintf(dbuf, sizeof(dbuf), "%s%d", day_names[now_tm.tm_wday], now_tm.tm_mday);
      lv_label_set_text(label_date, dbuf);
    } else {
      if (time_sync_format_date(dbuf, sizeof(dbuf))) {
        lv_label_set_text(label_date, dbuf);
      } else {
        lv_label_set_text(label_date, "-----");
      }
    }
  } else {
    if (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL &&
        digital_face &&
        !digital_face->split_time &&
        !digital_face->show_seconds) {
      lv_label_set_text(label_time, "--:--");
    } else {
      lv_label_set_text(label_time, "--:--:--");
    }
    lv_label_set_text(label_hour, "--");
    lv_label_set_text(label_minute, "--");
    lv_label_set_text(label_date, "No Time.");
  }

  sync_face_configuration();
  update_digital_date_box(digital_face);
  update_chronos_seconds_ring(
    qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL &&
    digital_face &&
    digital_face->show_rings,
    have_time ? now_tm.tm_hour : 0,
    have_time ? now_tm.tm_min : 0,
    have_time ? now_tm.tm_sec : 0
  );
  update_radio_icons();
  update_battery_icon();
  update_weather();
  update_analogue_date();

  if (analogue_watch_should_run()) {
    update_analogue_hands();
  }
}

static lv_timer_t* watch_tick_timer = nullptr;

static void tick_cb(lv_timer_t *timer) {
  (void)timer;
  update_watch_ui(false);
}

} // namespace

lv_obj_t* ui_watch_create_screen() {
  lv_obj_t *scr = lv_obj_create(NULL);
  watch_screen = scr;

  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(scr, WATCH_SIZE, WATCH_SIZE);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_style_radius(scr, 0, 0);
  lv_obj_set_style_pad_all(scr, 0, 0);

  digital_panel = create_panel(scr);

  digital_bg = lv_img_create(digital_panel);
  lv_img_set_src(digital_bg, &neon);
  lv_obj_center(digital_bg);

  label_time = lv_label_create(digital_panel);
  lv_label_set_text(label_time, "00:00:00");
  lv_obj_set_style_text_color(label_time, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_time, &Comfortaa_56, 0);

  label_hour = lv_label_create(digital_panel);
  lv_label_set_text(label_hour, "00");
  lv_obj_set_style_text_color(label_hour, lv_color_make(255, 220, 0), 0);
  lv_obj_set_style_text_font(label_hour, &orbitron, 0);
  lv_obj_add_flag(label_hour, LV_OBJ_FLAG_HIDDEN);

  label_minute = lv_label_create(digital_panel);
  lv_label_set_text(label_minute, "00");
  lv_obj_set_style_text_color(label_minute, lv_color_make(255, 220, 0), 0);
  lv_obj_set_style_text_font(label_minute, &orbitron, 0);
  lv_obj_add_flag(label_minute, LV_OBJ_FLAG_HIDDEN);

  setup_chronos_seconds_ring();

  chronos_date_box = lv_obj_create(digital_panel);
  lv_obj_set_style_bg_opa(chronos_date_box, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(chronos_date_box, 2, 0);
  lv_obj_set_style_border_color(chronos_date_box, lv_color_make(255, 220, 0), 0);
  lv_obj_set_style_radius(chronos_date_box, 10, 0);
  lv_obj_set_style_pad_all(chronos_date_box, 0, 0);
  lv_obj_clear_flag(chronos_date_box, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_clear_flag(chronos_date_box, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(chronos_date_box, LV_OBJ_FLAG_HIDDEN);

  label_date = lv_label_create(digital_panel);
  lv_label_set_text(label_date, "-----");
  lv_obj_set_style_text_color(label_date, lv_color_make(180, 180, 180), 0);
  lv_obj_set_style_text_font(label_date, &lv_font_montserrat_22, 0);

  label_weather_icon = lv_label_create(digital_panel);
  lv_label_set_text(label_weather_icon, "\uf07b");
  lv_obj_set_style_text_color(label_weather_icon, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_weather_icon, &weather_icons, 0);

  label_weather_temp = lv_label_create(digital_panel);
  lv_label_set_text(label_weather_temp, "--\xC2\xB0");
  lv_obj_set_style_text_color(label_weather_temp, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_weather_temp, &lv_font_montserrat_18, 0);

  label_battery = lv_label_create(digital_panel);
  lv_label_set_text(label_battery, LV_SYMBOL_BATTERY_FULL);
  lv_obj_set_style_text_color(label_battery, lv_color_make(0, 255, 0), 0);
  lv_obj_set_style_text_font(label_battery, &lv_font_montserrat_20, 0);

  label_wifi = lv_label_create(digital_panel);
  lv_label_set_text(label_wifi, LV_SYMBOL_WIFI);
  lv_obj_set_style_text_color(label_wifi, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_wifi, &lv_font_montserrat_18, 0);

  label_bt = lv_label_create(digital_panel);
  lv_label_set_text(label_bt, LV_SYMBOL_BLUETOOTH);
  lv_obj_set_style_text_color(label_bt, lv_color_white(), 0);
  lv_obj_set_style_text_font(label_bt, &lv_font_montserrat_18, 0);

  analogue_panel = create_panel(scr);

  analogue_bg = lv_img_create(analogue_panel);
  lv_img_set_src(analogue_bg, &feather);
  lv_obj_center(analogue_bg);

  analogue_date_label = lv_label_create(analogue_panel);
  lv_label_set_text(analogue_date_label, "--");
  lv_obj_set_style_text_color(analogue_date_label, lv_color_white(), 0);
  lv_obj_set_style_text_font(analogue_date_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_align(analogue_date_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(analogue_date_label, LV_ALIGN_CENTER, 0, -72);
  lv_obj_add_flag(analogue_date_label, LV_OBJ_FLAG_HIDDEN);

  hour_hand = lv_line_create(analogue_panel);
  minute_hand = lv_line_create(analogue_panel);
  second_hand = lv_line_create(analogue_panel);

  centre_dot = lv_obj_create(analogue_panel);
  lv_obj_set_style_radius(centre_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(centre_dot, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(centre_dot, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(centre_dot, 0, 0);
  lv_obj_set_style_shadow_width(centre_dot, 0, 0);
  lv_obj_set_style_pad_all(centre_dot, 0, 0);
  lv_obj_align(centre_dot, LV_ALIGN_CENTER, 0, 0);

  lv_obj_move_foreground(hour_hand);
  lv_obj_move_foreground(minute_hand);
  lv_obj_move_foreground(second_hand);
  lv_obj_move_foreground(centre_dot);
  lv_obj_move_foreground(analogue_date_label);

  style_hand(hour_hand, 5, lv_color_white(), false);
  style_hand(minute_hand, 3, lv_color_white(), false);
  style_hand(second_hand, 2, lv_color_white(), false);

  g_applied_mode = (ClockMode)-1;
  g_applied_digital_face = -1;
  g_applied_analogue_face = -1;
  g_analogue_angles_initialised = false;

  sync_face_configuration();
  tick_cb(nullptr);
  if (!watch_tick_timer) {
    watch_tick_timer = lv_timer_create(tick_cb, 1000, NULL);
  }
  return scr;
}

void ui_watch_refresh_now() {
  update_watch_ui(true);
}

void ui_watch_pause_timer() {
  if (watch_tick_timer) {
    lv_timer_pause(watch_tick_timer);
  }
}

void ui_watch_resume_timer() {
  if (watch_tick_timer) {
    lv_timer_resume(watch_tick_timer);
  }
  ui_watch_refresh_now();
}
