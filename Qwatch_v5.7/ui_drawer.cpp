#include <lvgl.h>
#include <stdint.h>
#include "ui_drawer.h"
#include "app_manager.h"

static constexpr int16_t TAP_MOVE_THRESHOLD_PX = 14;

static bool g_drawer_press_active = false;
static AppId g_drawer_pressed_id = APP_DRAWER;
static lv_point_t g_drawer_press_point = {120, 120};

static void style_round_btn(lv_obj_t* btn) {
  lv_obj_set_size(btn, 86, 86);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(btn, lv_color_make(30, 30, 30), 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
}

static void add_icon_label(lv_obj_t* btn, const char* sym, const char* text) {
  lv_obj_t* icon = lv_label_create(btn);
  lv_label_set_text(icon, sym);
  lv_obj_set_style_text_font(icon, &lv_font_montserrat_28, 0);
  lv_obj_center(icon);
  lv_obj_align(icon, LV_ALIGN_CENTER, 0, -10);

  lv_obj_t* lbl = lv_label_create(btn);
  lv_label_set_text(lbl, text);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
  lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 26);
}

static void launch_drawer_app(AppId id) {
  switch (id) {
    case APP_WATCHFACE:
      app_manager_show_animated(APP_WATCHFACE, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
      break;

    case APP_STOPWATCH:
      app_manager_show_animated(APP_STOPWATCH, LV_SCR_LOAD_ANIM_MOVE_LEFT);
      break;

    case APP_SETTINGS:
      app_manager_show_animated(APP_SETTINGS, LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
      break;

    case APP_PEDOMETER:
      app_manager_show_animated(APP_PEDOMETER, LV_SCR_LOAD_ANIM_MOVE_LEFT);
      break;

    case APP_DRAWER:
    default:
      app_manager_show(APP_WATCHFACE);
      break;
  }
}

static void on_btn_event(lv_event_t* e) {
  AppId id = (AppId)(intptr_t)lv_event_get_user_data(e);
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_PRESSED) {
    lv_indev_t* indev = lv_indev_get_act();
    if (indev) {
      lv_indev_get_point(indev, &g_drawer_press_point);
      g_drawer_press_active = true;
      g_drawer_pressed_id = id;
    }
    return;
  }

  if (code == LV_EVENT_PRESS_LOST || code == LV_EVENT_RELEASED) {
    lv_indev_t* indev = lv_indev_get_act();
    if (!g_drawer_press_active || g_drawer_pressed_id != id || indev == nullptr) {
      g_drawer_press_active = false;
      return;
    }

    lv_point_t release_point;
    lv_indev_get_point(indev, &release_point);

    const int16_t dx = release_point.x - g_drawer_press_point.x;
    const int16_t dy = release_point.y - g_drawer_press_point.y;
    const int16_t adx = dx < 0 ? -dx : dx;
    const int16_t ady = dy < 0 ? -dy : dy;

    g_drawer_press_active = false;

    // Only treat as a button activation if it was really a tap,
    // not a drag / swipe across the button.
    if (adx <= TAP_MOVE_THRESHOLD_PX && ady <= TAP_MOVE_THRESHOLD_PX) {
      launch_drawer_app(id);
    }
    return;
  }
}

lv_obj_t* ui_drawer_create_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_set_size(scr, 240, 240);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  // Title
  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Apps");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Top-left: Watch
  lv_obj_t* b_watch = lv_btn_create(scr);
  style_round_btn(b_watch);
  lv_obj_align(b_watch, LV_ALIGN_CENTER, -55, -45);
  add_icon_label(b_watch, LV_SYMBOL_BELL, "Watch");
  lv_obj_add_event_cb(b_watch, on_btn_event, LV_EVENT_PRESSED,   (void*)(intptr_t)APP_WATCHFACE);
  lv_obj_add_event_cb(b_watch, on_btn_event, LV_EVENT_RELEASED,  (void*)(intptr_t)APP_WATCHFACE);
  lv_obj_add_event_cb(b_watch, on_btn_event, LV_EVENT_PRESS_LOST,(void*)(intptr_t)APP_WATCHFACE);

  // Top-right: Stopwatch
  lv_obj_t* b_sw = lv_btn_create(scr);
  style_round_btn(b_sw);
  lv_obj_align(b_sw, LV_ALIGN_CENTER, 55, -45);
  add_icon_label(b_sw, LV_SYMBOL_REFRESH, "Stopwatch");
  lv_obj_add_event_cb(b_sw, on_btn_event, LV_EVENT_PRESSED,   (void*)(intptr_t)APP_STOPWATCH);
  lv_obj_add_event_cb(b_sw, on_btn_event, LV_EVENT_RELEASED,  (void*)(intptr_t)APP_STOPWATCH);
  lv_obj_add_event_cb(b_sw, on_btn_event, LV_EVENT_PRESS_LOST,(void*)(intptr_t)APP_STOPWATCH);

  // Bottom-left: Pedometer
  lv_obj_t* b_pedo = lv_btn_create(scr);
  style_round_btn(b_pedo);
  lv_obj_align(b_pedo, LV_ALIGN_CENTER, -55, 45);
  add_icon_label(b_pedo, LV_SYMBOL_UP, "Steps");
  lv_obj_add_event_cb(b_pedo, on_btn_event, LV_EVENT_PRESSED,   (void*)(intptr_t)APP_PEDOMETER);
  lv_obj_add_event_cb(b_pedo, on_btn_event, LV_EVENT_RELEASED,  (void*)(intptr_t)APP_PEDOMETER);
  lv_obj_add_event_cb(b_pedo, on_btn_event, LV_EVENT_PRESS_LOST,(void*)(intptr_t)APP_PEDOMETER);

  // Bottom-right: Settings
  lv_obj_t* b_set = lv_btn_create(scr);
  style_round_btn(b_set);
  lv_obj_align(b_set, LV_ALIGN_CENTER, 55, 45);
  add_icon_label(b_set, LV_SYMBOL_SETTINGS, "Settings");
  lv_obj_add_event_cb(b_set, on_btn_event, LV_EVENT_PRESSED,   (void*)(intptr_t)APP_SETTINGS);
  lv_obj_add_event_cb(b_set, on_btn_event, LV_EVENT_RELEASED,  (void*)(intptr_t)APP_SETTINGS);
  lv_obj_add_event_cb(b_set, on_btn_event, LV_EVENT_PRESS_LOST,(void*)(intptr_t)APP_SETTINGS);

  // Hint
  lv_obj_t* hint = lv_label_create(scr);
  lv_label_set_text(hint, "Swipe down = watch");
  lv_obj_set_style_text_font(hint, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(hint, lv_color_make(140, 140, 140), 0);
  lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -10);

  return scr;
}
