#include <lvgl.h>
#include <WiFi.h>
#include <stdio.h>

#include "ui_settings.h"
#include "app_manager.h"
#include "pedometer.h"
#include "qwatch_settings.h"
#include "time_sync.h"
#include "watch_faces.h"

namespace {

constexpr int SCREEN_SIZE = 240;
constexpr int16_t TAP_MOVE_THRESHOLD_PX = 14;
constexpr uint32_t WIFI_CREDENTIALS_ANIM_TIME_MS = 220;
constexpr uint32_t FACE_GALLERY_SWIPE_COOLDOWN_MS = 500;

static lv_obj_t* settings_screen = nullptr;
static lv_obj_t* menu_panel = nullptr;
static lv_obj_t* clock_panel = nullptr;
static lv_obj_t* wifi_panel = nullptr;
static lv_obj_t* steps_panel = nullptr;
static lv_obj_t* bluetooth_panel = nullptr;
static lv_obj_t* wifi_credentials_panel = nullptr;

static lv_obj_t* clock_toggle = nullptr;
static lv_obj_t* clock_toggle_knob = nullptr;
static lv_obj_t* steps_toggle = nullptr;
static lv_obj_t* steps_toggle_knob = nullptr;
static lv_obj_t* clock_left_label = nullptr;
static lv_obj_t* clock_right_label = nullptr;
static lv_obj_t* steps_left_label = nullptr;
static lv_obj_t* steps_right_label = nullptr;
static lv_obj_t* wifi_status_label = nullptr;
static lv_obj_t* wifi_signal_label = nullptr;
static lv_obj_t* wifi_connect_btn = nullptr;
static lv_obj_t* wifi_disconnect_btn = nullptr;
static lv_obj_t* wifi_ssid_label = nullptr;
static lv_obj_t* wifi_password_label = nullptr;
static lv_obj_t* wifi_ssid_ta = nullptr;
static lv_obj_t* wifi_password_ta = nullptr;
static lv_obj_t* wifi_keyboard = nullptr;
static lv_obj_t* bluetooth_toggle = nullptr;
static lv_obj_t* bluetooth_toggle_knob = nullptr;
static lv_obj_t* bluetooth_left_label = nullptr;
static lv_obj_t* bluetooth_right_label = nullptr;
static lv_obj_t* bluetooth_status_label = nullptr;
static lv_obj_t* battery_voltage_label = nullptr;

static lv_obj_t* face_background_img = nullptr;
static lv_obj_t* face_name_label = nullptr;
static const lv_img_dsc_t* face_current_img_src = nullptr;
static int face_browse_index = 0;
static bool face_gallery_active = false;
static uint32_t face_last_swipe_ms = 0;

static bool g_settings_menu_press_active = false;
static lv_obj_t* g_settings_menu_pressed_panel = nullptr;
static lv_point_t g_settings_menu_press_point = {120, 120};

static void update_wifi_panel() {
  if (!wifi_status_label || !wifi_signal_label) return;

  const bool wifi_active = time_sync_wifi_is_active();
  const bool wifi_connected = (WiFi.status() == WL_CONNECTED);

  if (!wifi_active) {
    lv_label_set_text(wifi_status_label, "Wi-Fi Off");
    lv_label_set_text(wifi_signal_label, "Radio disabled");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_make(180, 180, 180), 0);
    lv_obj_set_style_text_color(wifi_signal_label, lv_color_make(140, 140, 140), 0);
  } else if (wifi_connected) {
    char buf[64];
    const long rssi = WiFi.RSSI();
    snprintf(buf, sizeof(buf), "Signal: %ld dBm", rssi);

    lv_label_set_text(wifi_status_label, "Connected");
    lv_label_set_text(wifi_signal_label, buf);
    lv_obj_set_style_text_color(wifi_status_label, lv_color_white(), 0);
    lv_obj_set_style_text_color(wifi_signal_label, lv_color_white(), 0);
  } else {
    lv_label_set_text(wifi_status_label, "Disconnected");
    lv_label_set_text(wifi_signal_label, "Radio on");
    lv_obj_set_style_text_color(wifi_status_label, lv_color_make(200, 200, 200), 0);
    lv_obj_set_style_text_color(wifi_signal_label, lv_color_make(160, 160, 160), 0);
  }

  if (wifi_connect_btn) {
    lv_obj_set_style_bg_color(
      wifi_connect_btn,
      (!wifi_active || !wifi_connected) ? lv_color_make(30, 90, 180) : lv_color_make(55, 55, 55),
      0
    );
  }

  if (wifi_disconnect_btn) {
    lv_obj_set_style_bg_color(
      wifi_disconnect_btn,
      wifi_active ? lv_color_make(80, 40, 40) : lv_color_make(55, 55, 55),
      0
    );
  }
}

static void update_battery_voltage_label() {
  if (!battery_voltage_label) return;

  char buf[16];
  snprintf(buf, sizeof(buf), "%.3fV", time_sync_battery_voltage());
  lv_label_set_text(battery_voltage_label, buf);
}

static void update_bluetooth_toggle_labels() {
  if (!bluetooth_left_label || !bluetooth_right_label) return;

  const bool active = qwatch_settings_get_bluetooth_active();
  lv_obj_set_style_text_color(bluetooth_left_label,
                              active ? lv_color_white() : lv_color_make(130, 130, 130),
                              0);
  lv_obj_set_style_text_color(bluetooth_right_label,
                              active ? lv_color_make(130, 130, 130) : lv_color_white(),
                              0);
}

static void sync_bluetooth_toggle() {
  if (!bluetooth_toggle || !bluetooth_toggle_knob || !bluetooth_status_label) return;

  const bool active = qwatch_settings_get_bluetooth_active();
  lv_obj_set_style_bg_color(bluetooth_toggle,
                            active ? lv_color_make(40, 90, 190) : lv_color_make(70, 70, 70),
                            0);
  lv_obj_align(bluetooth_toggle_knob, active ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID,
               active ? 5 : -5, 0);
  lv_label_set_text(bluetooth_status_label, active ? "Status: Active" : "Status: Inactive");
  lv_obj_set_style_text_color(bluetooth_status_label,
                              active ? lv_color_make(80, 160, 255) : lv_color_make(180, 180, 180),
                              0);
  update_bluetooth_toggle_labels();
}

static void wifi_timer_cb(lv_timer_t* timer) {
  (void)timer;
  update_wifi_panel();
  sync_bluetooth_toggle();
  update_battery_voltage_label();
}

static void style_round_btn(lv_obj_t* btn) {
  lv_obj_set_size(btn, 78, 78);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(btn, lv_color_make(30, 30, 30), 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
}

static void add_icon_label(lv_obj_t* btn, const char* sym, const char* text) {
  lv_obj_t* icon = lv_label_create(btn);
  lv_label_set_text(icon, sym);
  lv_obj_set_style_text_font(icon, &lv_font_montserrat_26, 0);
  lv_obj_align(icon, LV_ALIGN_CENTER, 0, -9);

  lv_obj_t* lbl = lv_label_create(btn);
  lv_label_set_text(lbl, text);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 24);
}

static void style_small_round_btn(lv_obj_t* btn) {
  lv_obj_set_size(btn, 46, 46);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(btn, lv_color_make(45, 45, 45), 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
}

static void style_action_btn(lv_obj_t* btn, int w, int h) {
  lv_obj_set_size(btn, w, h);
  lv_obj_set_style_radius(btn, 18, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(btn, lv_color_make(30, 30, 30), 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
}

static void style_label_backplate(lv_obj_t* label) {
  lv_obj_set_style_bg_opa(label, LV_OPA_80, 0);
  lv_obj_set_style_bg_color(label, lv_color_make(35, 35, 35), 0);
  lv_obj_set_style_radius(label, 8, 0);
  lv_obj_set_style_pad_left(label, 8, 0);
  lv_obj_set_style_pad_right(label, 8, 0);
  lv_obj_set_style_pad_top(label, 3, 0);
  lv_obj_set_style_pad_bottom(label, 3, 0);
}

static void set_panel_y(void* obj, int32_t value) {
  lv_obj_set_y((lv_obj_t*)obj, value);
}

static lv_obj_t* g_wifi_credentials_panel_to_hide = nullptr;
static void show_wifi_ssid_entry();
static void show_wifi_password_entry();


static void hide_panel_on_anim_complete(lv_anim_t* anim) {
  (void)anim;
  if (!g_wifi_credentials_panel_to_hide) return;
  lv_obj_add_flag(g_wifi_credentials_panel_to_hide, LV_OBJ_FLAG_HIDDEN);
  g_wifi_credentials_panel_to_hide = nullptr;
}

static void animate_wifi_credentials_panel(int32_t from_y, int32_t to_y, bool hide_when_done) {
  if (!wifi_credentials_panel) return;

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, wifi_credentials_panel);
  lv_anim_set_exec_cb(&a, set_panel_y);
  lv_anim_set_values(&a, from_y, to_y);
  lv_anim_set_time(&a, WIFI_CREDENTIALS_ANIM_TIME_MS);
  if (hide_when_done) {
    g_wifi_credentials_panel_to_hide = wifi_credentials_panel;
    lv_anim_set_ready_cb(&a, hide_panel_on_anim_complete);
  }
  lv_anim_start(&a);
}

static void open_wifi_credentials_panel() {
  if (!wifi_panel || !wifi_credentials_panel) return;

  Serial.println("[wifi-ui] Opening Wi-Fi credentials panel");

  if (wifi_ssid_ta) {
    lv_textarea_set_text(wifi_ssid_ta, qwatch_settings_get_wifi_ssid());
  }

  if (wifi_password_ta) {
    lv_textarea_set_text(wifi_password_ta, qwatch_settings_get_wifi_password());
  }

  lv_obj_clear_flag(wifi_panel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(wifi_credentials_panel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_y(wifi_credentials_panel, -SCREEN_SIZE);
  animate_wifi_credentials_panel(-SCREEN_SIZE, 0, false);

  show_wifi_ssid_entry();
}

static void close_wifi_credentials_panel() {
  if (!wifi_panel || !wifi_credentials_panel) return;

  lv_obj_clear_flag(wifi_panel, LV_OBJ_FLAG_HIDDEN);
  animate_wifi_credentials_panel(lv_obj_get_y(wifi_credentials_panel), -SCREEN_SIZE, true);

  if (wifi_keyboard) {
    lv_keyboard_set_textarea(wifi_keyboard, nullptr);
  }
}

static void show_wifi_ssid_entry() {
  if (wifi_ssid_label) lv_obj_clear_flag(wifi_ssid_label, LV_OBJ_FLAG_HIDDEN);
  if (wifi_ssid_ta) lv_obj_clear_flag(wifi_ssid_ta, LV_OBJ_FLAG_HIDDEN);
  if (wifi_password_label) lv_obj_add_flag(wifi_password_label, LV_OBJ_FLAG_HIDDEN);
  if (wifi_password_ta) lv_obj_add_flag(wifi_password_ta, LV_OBJ_FLAG_HIDDEN);

  if (wifi_keyboard && wifi_ssid_ta) {
    lv_keyboard_set_textarea(wifi_keyboard, wifi_ssid_ta);
  }
}

static void show_wifi_password_entry() {
  if (wifi_ssid_label) lv_obj_add_flag(wifi_ssid_label, LV_OBJ_FLAG_HIDDEN);
  if (wifi_ssid_ta) lv_obj_add_flag(wifi_ssid_ta, LV_OBJ_FLAG_HIDDEN);
  if (wifi_password_label) lv_obj_clear_flag(wifi_password_label, LV_OBJ_FLAG_HIDDEN);
  if (wifi_password_ta) lv_obj_clear_flag(wifi_password_ta, LV_OBJ_FLAG_HIDDEN);

  if (wifi_keyboard && wifi_password_ta) {
    lv_keyboard_set_textarea(wifi_keyboard, wifi_password_ta);
  }
}

static void show_panel(lv_obj_t* panel) {
  face_gallery_active = (panel == clock_panel);

  if (menu_panel) lv_obj_add_flag(menu_panel, LV_OBJ_FLAG_HIDDEN);
  if (clock_panel) lv_obj_add_flag(clock_panel, LV_OBJ_FLAG_HIDDEN);
  if (wifi_panel) lv_obj_add_flag(wifi_panel, LV_OBJ_FLAG_HIDDEN);
  if (wifi_credentials_panel) lv_obj_add_flag(wifi_credentials_panel, LV_OBJ_FLAG_HIDDEN);
  if (steps_panel) lv_obj_add_flag(steps_panel, LV_OBJ_FLAG_HIDDEN);
  if (bluetooth_panel) lv_obj_add_flag(bluetooth_panel, LV_OBJ_FLAG_HIDDEN);

  if (panel) {
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_HIDDEN);
  }
}

static lv_obj_t* create_binary_toggle(lv_obj_t* parent, lv_obj_t** knob_out) {
  lv_obj_t* toggle = lv_btn_create(parent);
  lv_obj_set_size(toggle, 56, 28);
  lv_obj_set_style_radius(toggle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(toggle, 0, 0);
  lv_obj_set_style_shadow_width(toggle, 0, 0);
  lv_obj_set_style_bg_opa(toggle, LV_OPA_COVER, 0);
  lv_obj_clear_flag(toggle, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(toggle, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_pad_all(toggle, 0, 0);

  lv_obj_t* knob = lv_obj_create(toggle);
  lv_obj_set_size(knob, 20, 20);
  lv_obj_set_style_radius(knob, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(knob, 0, 0);
  lv_obj_set_style_shadow_width(knob, 0, 0);
  lv_obj_set_style_bg_color(knob, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(knob, LV_OPA_COVER, 0);
  lv_obj_clear_flag(knob, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

  if (knob_out) {
    *knob_out = knob;
  }
  return toggle;
}

static void update_clock_toggle_labels() {
  const bool digital = (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL);
  if (clock_left_label) {
    lv_obj_set_style_text_color(clock_left_label, digital ? lv_color_white() : lv_color_make(120, 120, 120), 0);
  }
  if (clock_right_label) {
    lv_obj_set_style_text_color(clock_right_label, digital ? lv_color_make(120, 120, 120) : lv_color_white(), 0);
  }
}

static void update_steps_toggle_labels() {
  const bool active = qwatch_settings_get_steps_active();
  if (steps_left_label) {
    lv_obj_set_style_text_color(steps_left_label, active ? lv_color_white() : lv_color_make(120, 120, 120), 0);
  }
  if (steps_right_label) {
    lv_obj_set_style_text_color(steps_right_label, active ? lv_color_make(120, 120, 120) : lv_color_white(), 0);
  }
}

static void sync_clock_toggle() {
  if (!clock_toggle || !clock_toggle_knob) return;

  const bool digital = (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL);
  lv_obj_set_style_bg_color(clock_toggle,
                            digital ? lv_color_make(70, 70, 70) : lv_color_make(80, 120, 80),
                            0);
  lv_obj_align(clock_toggle_knob, digital ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID,
               digital ? 4 : -4, 0);
  update_clock_toggle_labels();
}

static void sync_steps_toggle() {
  if (!steps_toggle || !steps_toggle_knob) return;

  const bool active = qwatch_settings_get_steps_active();
  lv_obj_set_style_bg_color(steps_toggle,
                            active ? lv_color_make(80, 120, 80) : lv_color_make(70, 70, 70),
                            0);
  lv_obj_align(steps_toggle_knob, active ? LV_ALIGN_LEFT_MID : LV_ALIGN_RIGHT_MID,
               active ? 5 : -5, 0);
  update_steps_toggle_labels();
}

static int current_face_count() {
  return (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL)
    ? watch_faces_get_digital_count()
    : watch_faces_get_analogue_count();
}

static int selected_face_index() {
  return (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL)
    ? qwatch_settings_get_digital_face_index()
    : qwatch_settings_get_analogue_face_index();
}

static void set_selected_face_index(int index) {
  if (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL) {
    qwatch_settings_set_digital_face_index(index);
  } else {
    qwatch_settings_set_analogue_face_index(index);
  }
}

static int wrap_index(int index, int count) {
  if (count <= 0) return 0;
  while (index < 0) index += count;
  while (index >= count) index -= count;
  return index;
}

static const char* current_browsed_face_name() {
  if (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL) {
    const DigitalWatchFaceDef* face = watch_faces_get_digital(face_browse_index);
    return face ? face->name : nullptr;
  }
  const AnalogueWatchFaceDef* face = watch_faces_get_analogue(face_browse_index);
  return face ? face->name : nullptr;
}

static const lv_img_dsc_t* current_browsed_face_image() {
  if (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL) {
    const DigitalWatchFaceDef* face = watch_faces_get_digital(face_browse_index);
    return face ? face->image : nullptr;
  }

  const AnalogueWatchFaceDef* face = watch_faces_get_analogue(face_browse_index);
  return face ? face->image : nullptr;
}

static void refresh_face_gallery() {
  if (!face_background_img) {
    return;
  }

  const int count = current_face_count();

  if (count <= 0) {
    face_current_img_src = nullptr;
    lv_obj_add_flag(face_background_img, LV_OBJ_FLAG_HIDDEN);
    if (face_name_label) lv_label_set_text(face_name_label, "No faces");
    return;
  }

  face_browse_index = wrap_index(face_browse_index, count);

  const lv_img_dsc_t* img = current_browsed_face_image();
  if (img) {
    if (img != face_current_img_src) {
      lv_img_set_src(face_background_img, img);
      face_current_img_src = img;
    }
    lv_obj_clear_flag(face_background_img, LV_OBJ_FLAG_HIDDEN);
  } else {
    face_current_img_src = nullptr;
    lv_obj_add_flag(face_background_img, LV_OBJ_FLAG_HIDDEN);
  }

  if (face_name_label) {
    const char* name = current_browsed_face_name();
    lv_label_set_text(face_name_label, name ? name : "Face");
  }
}

static void sync_face_browser_to_selected() {
  face_last_swipe_ms = 0;
  face_browse_index = selected_face_index();
  refresh_face_gallery();
}

static void move_face_browser(int delta) {
  if (delta == 0) return;


  const int count = current_face_count();
  if (count <= 1) return;

  const int direction = (delta > 0) ? 1 : -1;
  face_browse_index = wrap_index(face_browse_index + direction, count);
  refresh_face_gallery();
}

static void handle_face_browser_swipe(int delta) {
  if (delta == 0) return;
  if (!face_gallery_active) return;

  const uint32_t now = millis();
  if (face_last_swipe_ms != 0 && (now - face_last_swipe_ms) < FACE_GALLERY_SWIPE_COOLDOWN_MS) {
    return;
  }

  face_last_swipe_ms = now;
  move_face_browser((delta > 0) ? 1 : -1);
}

static void select_browsed_face() {
  set_selected_face_index(face_browse_index);
  refresh_face_gallery();
}

static void on_back_to_menu(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  show_panel(menu_panel);
}

static void on_open_panel(lv_event_t* e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* panel = (lv_obj_t*)lv_event_get_user_data(e);

  if (code == LV_EVENT_PRESSED) {
    lv_indev_t* indev = lv_indev_get_act();
    if (indev) {
      lv_indev_get_point(indev, &g_settings_menu_press_point);
      g_settings_menu_press_active = true;
      g_settings_menu_pressed_panel = panel;
    }
    return;
  }

  if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_RELEASED) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!g_settings_menu_press_active || g_settings_menu_pressed_panel != panel || indev == nullptr) {
      g_settings_menu_press_active = false;
      return;
    }

    lv_point_t release_point;
    lv_indev_get_point(indev, &release_point);

    const int16_t dx = release_point.x - g_settings_menu_press_point.x;
    const int16_t dy = release_point.y - g_settings_menu_press_point.y;
    const int16_t adx = dx < 0 ? -dx : dx;
    const int16_t ady = dy < 0 ? -dy : dy;

    g_settings_menu_press_active = false;

    if (adx <= TAP_MOVE_THRESHOLD_PX && ady <= TAP_MOVE_THRESHOLD_PX) {
      if (panel == clock_panel) {
        sync_clock_toggle();
        sync_face_browser_to_selected();
      } else if (panel == wifi_panel) {
        update_wifi_panel();
      } else if (panel == steps_panel) {
        sync_steps_toggle();
      } else if (panel == bluetooth_panel) {
        sync_bluetooth_toggle();
      }
      show_panel(panel);
    }
  }
}

static void on_clock_toggle_pressed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  const bool digital = (qwatch_settings_get_clock_mode() == CLOCK_MODE_DIGITAL);
  qwatch_settings_set_clock_mode(digital ? CLOCK_MODE_ANALOGUE : CLOCK_MODE_DIGITAL);
  sync_clock_toggle();
  sync_face_browser_to_selected();
}

static void on_steps_toggle_pressed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  const bool new_active = !qwatch_settings_get_steps_active();
  qwatch_settings_set_steps_active(new_active);
  if (new_active) {
    pedometer_start();
  } else {
    pedometer_pause();
  }
  sync_steps_toggle();
}

static void on_reset_logs(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  pedometer_reset();
}

static void on_bluetooth_toggle_pressed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  const bool new_active = !qwatch_settings_get_bluetooth_active();
  if (time_sync_set_bluetooth_active(new_active)) {
    sync_bluetooth_toggle();
  }
}

static void on_wifi_credentials_textarea_focused(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_FOCUSED) return;
  if (!wifi_keyboard) return;

  lv_obj_t* target = lv_event_get_target(e);
  lv_keyboard_set_textarea(wifi_keyboard, target);
}

static bool persist_wifi_credentials_from_ui() {
  if (!wifi_ssid_ta || !wifi_password_ta) {
    Serial.println("[wifi-ui] Save aborted: textarea pointer missing");
    return false;
  }

  const char* ssid = lv_textarea_get_text(wifi_ssid_ta);
  const char* password = lv_textarea_get_text(wifi_password_ta);

  Serial.println("[wifi-ui] Persisting Wi-Fi credentials");

  qwatch_settings_set_wifi_ssid(ssid ? ssid : "");
  qwatch_settings_set_wifi_password(password ? password : "");

  const bool save_ok = qwatch_settings_save_wifi_credentials();
  Serial.print("[wifi-ui] qwatch_settings_save_wifi_credentials(): ");
  Serial.println(save_ok ? "SUCCESS" : "FAILED");

  if (!save_ok) {
    Serial.println("[wifi-ui] Credentials were not persisted. Device may connect now but credentials will not survive reboot.");
  }

  return save_ok;
}

static void on_wifi_credentials_textarea_ready(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_READY) return;

  lv_obj_t* target = lv_event_get_target(e);
  if (target == wifi_ssid_ta) {
    Serial.println("[wifi-ui] SSID entry complete; switching to password entry");
    show_wifi_password_entry();
    if (wifi_password_ta) {
      lv_keyboard_set_textarea(wifi_keyboard, wifi_password_ta);
      lv_obj_add_state(wifi_password_ta, LV_STATE_FOCUSED);
    }
    return;
  }

  if (target == wifi_password_ta) {
    Serial.println("[wifi-ui] Password entry complete; attempting save and connect");
    const bool save_ok = persist_wifi_credentials_from_ui();

    if (!save_ok) {
      Serial.println("[wifi-ui] Continuing with connect attempt using in-memory credentials");
    }

    time_sync_set_wifi_active(true);
    update_wifi_panel();
    close_wifi_credentials_panel();
  }
}

static void on_wifi_credentials_back_pressed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  close_wifi_credentials_panel();
}
static void on_wifi_connect_pressed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  if (time_sync_wifi_is_connected()) {
    update_wifi_panel();
    return;
  }

  open_wifi_credentials_panel();
}

static void on_wifi_disconnect_pressed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  time_sync_set_wifi_active(false);
  update_wifi_panel();
}

static void on_face_background_pressed(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  lv_obj_t* target = lv_event_get_target(e);
  if (target != face_background_img && target != clock_panel) return;
  select_browsed_face();
}

static lv_obj_t* create_subpage(lv_obj_t* parent, const char* title_text, int title_y = 10) {
  lv_obj_t* panel = lv_obj_create(parent);
  lv_obj_set_size(panel, SCREEN_SIZE, SCREEN_SIZE);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(panel, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(panel, 0, 0);
  lv_obj_set_style_radius(panel, 0, 0);
  lv_obj_center(panel);
  lv_obj_add_flag(panel, LV_OBJ_FLAG_HIDDEN);

  lv_obj_t* title = lv_label_create(panel);
  lv_label_set_text(title, title_text);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, title_y);

  lv_obj_t* btn_back = lv_btn_create(panel);
  style_small_round_btn(btn_back);
  lv_obj_align(btn_back, LV_ALIGN_TOP_MID, 0, 2);
  lv_obj_add_event_cb(btn_back, on_back_to_menu, LV_EVENT_CLICKED, NULL);

  lv_obj_t* lb = lv_label_create(btn_back);
  lv_label_set_text(lb, LV_SYMBOL_LEFT);
  lv_obj_center(lb);

  return panel;
}

} // namespace

lv_obj_t* ui_settings_create_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  settings_screen = scr;
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(scr, SCREEN_SIZE, SCREEN_SIZE);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_style_radius(scr, 0, 0);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Settings");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  menu_panel = lv_obj_create(scr);
  lv_obj_set_size(menu_panel, SCREEN_SIZE, SCREEN_SIZE);
  lv_obj_clear_flag(menu_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_opa(menu_panel, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(menu_panel, 0, 0);
  lv_obj_set_style_radius(menu_panel, 0, 0);
  lv_obj_center(menu_panel);

  lv_obj_t* b_clock = lv_btn_create(menu_panel);
  style_round_btn(b_clock);
  lv_obj_align(b_clock, LV_ALIGN_CENTER, -50, -45);
  add_icon_label(b_clock, LV_SYMBOL_BELL, "Watch");

  lv_obj_t* b_wifi = lv_btn_create(menu_panel);
  style_round_btn(b_wifi);
  lv_obj_align(b_wifi, LV_ALIGN_CENTER, 50, -45);
  add_icon_label(b_wifi, LV_SYMBOL_WIFI, "WiFi");

  lv_obj_t* b_steps = lv_btn_create(menu_panel);
  style_round_btn(b_steps);
  lv_obj_align(b_steps, LV_ALIGN_CENTER, -50, 45);
  add_icon_label(b_steps, LV_SYMBOL_UP, "Steps");

  lv_obj_t* b_bluetooth = lv_btn_create(menu_panel);
  style_round_btn(b_bluetooth);
  lv_obj_align(b_bluetooth, LV_ALIGN_CENTER, 50, 45);
  add_icon_label(b_bluetooth, LV_SYMBOL_BLUETOOTH, "Bluetooth");

  battery_voltage_label = lv_label_create(menu_panel);
  lv_label_set_text(battery_voltage_label, "--.--V");
  lv_obj_set_style_text_font(battery_voltage_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(battery_voltage_label, lv_color_white(), 0);
  lv_obj_align(battery_voltage_label, LV_ALIGN_CENTER, 0, 0);

  lv_obj_t* hint = lv_label_create(menu_panel);
  lv_label_set_text(hint, "Swipe up = watch");
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(hint, lv_color_make(140, 140, 140), 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

  clock_panel = create_subpage(scr, "Clock Settings", 40);
  wifi_panel = create_subpage(scr, "Wi-Fi Settings");
  steps_panel = create_subpage(scr, "Steps Settings");
  bluetooth_panel = create_subpage(scr, "Bluetooth Settings");

  lv_obj_add_event_cb(b_clock, on_open_panel, LV_EVENT_PRESSED, clock_panel);
  lv_obj_add_event_cb(b_clock, on_open_panel, LV_EVENT_RELEASED, clock_panel);
  lv_obj_add_event_cb(b_clock, on_open_panel, LV_EVENT_PRESS_LOST, clock_panel);
  lv_obj_add_event_cb(b_wifi, on_open_panel, LV_EVENT_PRESSED, wifi_panel);
  lv_obj_add_event_cb(b_wifi, on_open_panel, LV_EVENT_RELEASED, wifi_panel);
  lv_obj_add_event_cb(b_wifi, on_open_panel, LV_EVENT_PRESS_LOST, wifi_panel);
  lv_obj_add_event_cb(b_steps, on_open_panel, LV_EVENT_PRESSED, steps_panel);
  lv_obj_add_event_cb(b_steps, on_open_panel, LV_EVENT_RELEASED, steps_panel);
  lv_obj_add_event_cb(b_steps, on_open_panel, LV_EVENT_PRESS_LOST, steps_panel);
  lv_obj_add_event_cb(b_bluetooth, on_open_panel, LV_EVENT_PRESSED, bluetooth_panel);
  lv_obj_add_event_cb(b_bluetooth, on_open_panel, LV_EVENT_RELEASED, bluetooth_panel);
  lv_obj_add_event_cb(b_bluetooth, on_open_panel, LV_EVENT_PRESS_LOST, bluetooth_panel);

  face_background_img = lv_img_create(clock_panel);
  lv_obj_center(face_background_img);
  lv_img_set_zoom(face_background_img, LV_IMG_ZOOM_NONE);
  lv_obj_clear_flag(face_background_img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(face_background_img, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(face_background_img, on_face_background_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_move_background(face_background_img);

  lv_obj_t* clock_mode_bg = lv_obj_create(clock_panel);
  lv_obj_set_size(clock_mode_bg, 210, 34);
  lv_obj_set_style_bg_opa(clock_mode_bg, LV_OPA_80, 0);
  lv_obj_set_style_bg_color(clock_mode_bg, lv_color_make(35, 35, 35), 0);
  lv_obj_set_style_border_width(clock_mode_bg, 0, 0);
  lv_obj_set_style_radius(clock_mode_bg, 17, 0);
  lv_obj_set_style_pad_all(clock_mode_bg, 0, 0);
  lv_obj_clear_flag(clock_mode_bg, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(clock_mode_bg, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(clock_mode_bg, on_clock_toggle_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_align(clock_mode_bg, LV_ALIGN_CENTER, 0, -28);

  clock_left_label = lv_label_create(clock_panel);
  lv_label_set_text(clock_left_label, "Digital");
  lv_obj_set_style_text_font(clock_left_label, &lv_font_montserrat_12, 0);
  lv_obj_add_flag(clock_left_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(clock_left_label, on_clock_toggle_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_align(clock_left_label, LV_ALIGN_CENTER, -70, -28);

  clock_toggle = create_binary_toggle(clock_panel, &clock_toggle_knob);
  lv_obj_set_size(clock_toggle, 50, 24);
  lv_obj_set_size(clock_toggle_knob, 18, 18);
  lv_obj_align(clock_toggle, LV_ALIGN_CENTER, 0, -28);
  lv_obj_add_event_cb(clock_toggle, on_clock_toggle_pressed, LV_EVENT_CLICKED, NULL);

  clock_right_label = lv_label_create(clock_panel);
  lv_label_set_text(clock_right_label, "Analogue");
  lv_obj_set_style_text_font(clock_right_label, &lv_font_montserrat_12, 0);
  lv_obj_add_flag(clock_right_label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(clock_right_label, on_clock_toggle_pressed, LV_EVENT_CLICKED, NULL);
  lv_obj_align(clock_right_label, LV_ALIGN_CENTER, 70, -28);

  lv_obj_t* face_hint = lv_label_create(clock_panel);
  lv_label_set_text(face_hint, "Swipe to select");
  lv_obj_set_style_text_font(face_hint, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(face_hint, lv_color_make(210, 210, 210), 0);
  style_label_backplate(face_hint);
  lv_obj_align(face_hint, LV_ALIGN_TOP_MID, 0, 100);

  face_name_label = lv_label_create(clock_panel);
  lv_label_set_text(face_name_label, "Face");
  lv_obj_set_style_text_font(face_name_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(face_name_label, lv_color_white(), 0);
  style_label_backplate(face_name_label);
  lv_obj_align(face_name_label, LV_ALIGN_BOTTOM_MID, 0, -10);

  wifi_connect_btn = lv_btn_create(wifi_panel);
  style_action_btn(wifi_connect_btn, 88, 40);
  lv_obj_align(wifi_connect_btn, LV_ALIGN_CENTER, -50, -10);
  lv_obj_add_event_cb(wifi_connect_btn, on_wifi_connect_pressed, LV_EVENT_CLICKED, NULL);

  lv_obj_t* wifi_connect_lbl = lv_label_create(wifi_connect_btn);
  lv_label_set_text(wifi_connect_lbl, "Connect");
  lv_obj_set_style_text_font(wifi_connect_lbl, &lv_font_montserrat_14, 0);
  lv_obj_center(wifi_connect_lbl);

  wifi_disconnect_btn = lv_btn_create(wifi_panel);
  style_action_btn(wifi_disconnect_btn, 104, 40);
  lv_obj_align(wifi_disconnect_btn, LV_ALIGN_CENTER, 54, -10);
  lv_obj_add_event_cb(wifi_disconnect_btn, on_wifi_disconnect_pressed, LV_EVENT_CLICKED, NULL);

  lv_obj_t* wifi_disconnect_lbl = lv_label_create(wifi_disconnect_btn);
  lv_label_set_text(wifi_disconnect_lbl, "Disconnect");
  lv_obj_set_style_text_font(wifi_disconnect_lbl, &lv_font_montserrat_14, 0);
  lv_obj_center(wifi_disconnect_lbl);

  wifi_status_label = lv_label_create(wifi_panel);
  lv_label_set_text(wifi_status_label, "--");
  lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(wifi_status_label, LV_ALIGN_CENTER, 0, 34);

  wifi_signal_label = lv_label_create(wifi_panel);
  lv_label_set_text(wifi_signal_label, "--");
  lv_obj_set_style_text_font(wifi_signal_label, &lv_font_montserrat_12, 0);
  lv_obj_align(wifi_signal_label, LV_ALIGN_CENTER, 0, 56);

  wifi_credentials_panel = lv_obj_create(scr);
  lv_obj_set_size(wifi_credentials_panel, SCREEN_SIZE, SCREEN_SIZE);
  lv_obj_clear_flag(wifi_credentials_panel, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(wifi_credentials_panel, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(wifi_credentials_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(wifi_credentials_panel, 0, 0);
  lv_obj_set_style_radius(wifi_credentials_panel, 0, 0);
  lv_obj_add_flag(wifi_credentials_panel, LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_y(wifi_credentials_panel, -SCREEN_SIZE);

  lv_obj_t* wifi_credentials_back_btn = lv_btn_create(wifi_credentials_panel);
  style_small_round_btn(wifi_credentials_back_btn);
  lv_obj_align(wifi_credentials_back_btn, LV_ALIGN_TOP_MID, 0, 2);
  lv_obj_add_event_cb(wifi_credentials_back_btn, on_wifi_credentials_back_pressed, LV_EVENT_CLICKED, NULL);

  lv_obj_t* wifi_credentials_back_lbl = lv_label_create(wifi_credentials_back_btn);
  lv_label_set_text(wifi_credentials_back_lbl, LV_SYMBOL_LEFT);
  lv_obj_center(wifi_credentials_back_lbl);

  wifi_ssid_label = lv_label_create(wifi_credentials_panel);
  lv_label_set_text(wifi_ssid_label, "SSID");
  lv_obj_set_style_text_font(wifi_ssid_label, &lv_font_montserrat_12, 0);
  lv_obj_align(wifi_ssid_label, LV_ALIGN_TOP_LEFT, 2, 30);

  wifi_ssid_ta = lv_textarea_create(wifi_credentials_panel);
  lv_obj_set_size(wifi_ssid_ta, 200, 23);
  lv_obj_align(wifi_ssid_ta, LV_ALIGN_TOP_MID, 0, 44);
  lv_obj_set_style_text_font(wifi_ssid_ta, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_font(wifi_ssid_ta, &lv_font_montserrat_12, LV_PART_TEXTAREA_PLACEHOLDER);
  lv_textarea_set_one_line(wifi_ssid_ta, true);
  lv_textarea_set_placeholder_text(wifi_ssid_ta, "SSID");
  lv_obj_add_event_cb(wifi_ssid_ta, on_wifi_credentials_textarea_focused, LV_EVENT_FOCUSED, NULL);
  lv_obj_add_event_cb(wifi_ssid_ta, on_wifi_credentials_textarea_ready, LV_EVENT_READY, NULL);

  wifi_password_label = lv_label_create(wifi_credentials_panel);
  lv_label_set_text(wifi_password_label, "Password");
  lv_obj_set_style_text_font(wifi_password_label, &lv_font_montserrat_12, 0);
  lv_obj_align(wifi_password_label, LV_ALIGN_TOP_LEFT, 2, 30);
  lv_obj_add_flag(wifi_password_label, LV_OBJ_FLAG_HIDDEN);

  wifi_password_ta = lv_textarea_create(wifi_credentials_panel);
  lv_obj_set_size(wifi_password_ta, 200, 23);
  lv_obj_align(wifi_password_ta, LV_ALIGN_TOP_MID, 0, 44);
  lv_obj_set_style_text_font(wifi_password_ta, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_font(wifi_password_ta, &lv_font_montserrat_12, LV_PART_TEXTAREA_PLACEHOLDER);
  lv_textarea_set_one_line(wifi_password_ta, true);
  lv_textarea_set_password_mode(wifi_password_ta, false);
  lv_textarea_set_placeholder_text(wifi_password_ta, "Password");
  lv_obj_add_event_cb(wifi_password_ta, on_wifi_credentials_textarea_focused, LV_EVENT_FOCUSED, NULL);
  lv_obj_add_event_cb(wifi_password_ta, on_wifi_credentials_textarea_ready, LV_EVENT_READY, NULL);
  lv_obj_add_flag(wifi_password_ta, LV_OBJ_FLAG_HIDDEN);


  wifi_keyboard = lv_keyboard_create(wifi_credentials_panel);
  lv_obj_set_size(wifi_keyboard, (SCREEN_SIZE - 20), 100);
  lv_obj_align(wifi_keyboard, LV_ALIGN_TOP_MID, 0, 80);
  lv_keyboard_set_textarea(wifi_keyboard, wifi_ssid_ta);

  steps_left_label = lv_label_create(steps_panel);
  lv_label_set_text(steps_left_label, "Active");
  lv_obj_set_style_text_font(steps_left_label, &lv_font_montserrat_18, 0);
  lv_obj_align(steps_left_label, LV_ALIGN_CENTER, -72, -22);

  steps_toggle = create_binary_toggle(steps_panel, &steps_toggle_knob);
  lv_obj_align(steps_toggle, LV_ALIGN_CENTER, 0, -18);
  lv_obj_add_event_cb(steps_toggle, on_steps_toggle_pressed, LV_EVENT_CLICKED, NULL);

  steps_right_label = lv_label_create(steps_panel);
  lv_label_set_text(steps_right_label, "Inactive");
  lv_obj_set_style_text_font(steps_right_label, &lv_font_montserrat_18, 0);
  lv_obj_align(steps_right_label, LV_ALIGN_CENTER, 78, -22);

  lv_obj_t* btn_reset = lv_btn_create(steps_panel);
  style_action_btn(btn_reset, 132, 44);
  lv_obj_align(btn_reset, LV_ALIGN_CENTER, 0, 46);
  lv_obj_add_event_cb(btn_reset, on_reset_logs, LV_EVENT_CLICKED, NULL);

  lv_obj_t* reset_lbl = lv_label_create(btn_reset);
  lv_label_set_text(reset_lbl, "Reset Logs");
  lv_obj_set_style_text_font(reset_lbl, &lv_font_montserrat_16, 0);
  lv_obj_center(reset_lbl);

  bluetooth_left_label = lv_label_create(bluetooth_panel);
  lv_label_set_text(bluetooth_left_label, "Active");
  lv_obj_set_style_text_font(bluetooth_left_label, &lv_font_montserrat_18, 0);
  lv_obj_align(bluetooth_left_label, LV_ALIGN_CENTER, -72, -22);

  bluetooth_toggle = create_binary_toggle(bluetooth_panel, &bluetooth_toggle_knob);
  lv_obj_align(bluetooth_toggle, LV_ALIGN_CENTER, 0, -18);
  lv_obj_add_event_cb(bluetooth_toggle, on_bluetooth_toggle_pressed, LV_EVENT_CLICKED, NULL);

  bluetooth_right_label = lv_label_create(bluetooth_panel);
  lv_label_set_text(bluetooth_right_label, "Inactive");
  lv_obj_set_style_text_font(bluetooth_right_label, &lv_font_montserrat_18, 0);
  lv_obj_align(bluetooth_right_label, LV_ALIGN_CENTER, 82, -22);

  bluetooth_status_label = lv_label_create(bluetooth_panel);
  lv_label_set_text(bluetooth_status_label, "Status: Inactive");
  lv_obj_set_style_text_font(bluetooth_status_label, &lv_font_montserrat_20, 0);
  lv_obj_align(bluetooth_status_label, LV_ALIGN_CENTER, 0, 42);

  sync_clock_toggle();
  sync_steps_toggle();
  sync_bluetooth_toggle();
  sync_face_browser_to_selected();
  update_wifi_panel();
  update_battery_voltage_label();
  lv_timer_create(wifi_timer_cb, 1000, NULL);
  show_panel(menu_panel);

  return scr;
}


bool ui_settings_handle_swipe_up() {
  return face_gallery_active;
}

bool ui_settings_handle_swipe_down() {
  return face_gallery_active;
}

bool ui_settings_handle_swipe_left() {
  if (!face_gallery_active) return false;
  handle_face_browser_swipe(1);
  return true;
}

bool ui_settings_handle_swipe_right() {
  if (!face_gallery_active) return false;
  handle_face_browser_swipe(-1);
  return true;
}
