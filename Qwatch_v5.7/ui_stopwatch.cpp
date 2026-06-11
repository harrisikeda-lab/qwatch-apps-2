#include <lvgl.h>
#include <Arduino.h>
#include "ui_stopwatch.h"

static lv_obj_t* label_time = nullptr;
static lv_obj_t* btn_start = nullptr;
static lv_obj_t* btn_reset = nullptr;
static lv_obj_t* label_start = nullptr;

static bool running = false;
static uint32_t start_ms = 0;
static uint32_t elapsed_ms = 0;

static void format_sw(char* out, size_t n, uint32_t ms) {
  uint32_t total_cs = ms / 10;               // centiseconds
  uint32_t cs = total_cs % 100;
  uint32_t total_s = total_cs / 100;
  uint32_t s = total_s % 60;
  uint32_t m = (total_s / 60);

  snprintf(out, n, "%02lu:%02lu.%02lu", (unsigned long)m, (unsigned long)s, (unsigned long)cs);
}

static void update_label() {
  char buf[16];
  format_sw(buf, sizeof(buf), elapsed_ms);
  lv_label_set_text(label_time, buf);
}

static void tick_cb(lv_timer_t* t) {
  (void)t;
  if (!running) return;

  uint32_t now = millis();
  elapsed_ms = (now - start_ms);
  update_label();
}

static void on_start_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

  if (!running) {
    running = true;
    start_ms = millis() - elapsed_ms;
    lv_label_set_text(label_start, LV_SYMBOL_PAUSE);
  } else {
    running = false;
    elapsed_ms = millis() - start_ms;
    lv_label_set_text(label_start, LV_SYMBOL_PLAY);
  }
}

static void on_reset_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  running = false;
  elapsed_ms = 0;
  lv_label_set_text(label_start, LV_SYMBOL_PLAY);
  update_label();
}
static void round_btn(lv_obj_t* btn, int w, int h) {
  lv_obj_set_size(btn, w, h);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(btn, lv_color_make(30, 30, 30), 0);
}

lv_obj_t* ui_stopwatch_create_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(scr, 240, 240);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Stopwatch");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  label_time = lv_label_create(scr);
  lv_obj_set_style_text_font(label_time, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(label_time, lv_color_white(), 0);
  lv_obj_align(label_time, LV_ALIGN_CENTER, 0, -20);
  update_label();

  // Start/Pause
  btn_start = lv_btn_create(scr);
  round_btn(btn_start, 80, 80);
  lv_obj_align(btn_start, LV_ALIGN_CENTER, -55, 70);
  lv_obj_add_event_cb(btn_start, on_start_clicked, LV_EVENT_CLICKED, NULL);

  label_start = lv_label_create(btn_start);
  lv_label_set_text(label_start, LV_SYMBOL_PLAY);
  lv_obj_set_style_text_font(label_start, &lv_font_montserrat_32, 0);
  lv_obj_center(label_start);

  // Reset
  btn_reset = lv_btn_create(scr);
  round_btn(btn_reset, 80, 80);
  lv_obj_align(btn_reset, LV_ALIGN_CENTER, 55, 70);
  lv_obj_add_event_cb(btn_reset, on_reset_clicked, LV_EVENT_CLICKED, NULL);

  lv_obj_t* lbl_r = lv_label_create(btn_reset);
  lv_label_set_text(lbl_r, LV_SYMBOL_REFRESH);
  lv_obj_set_style_text_font(lbl_r, &lv_font_montserrat_32, 0);
  lv_obj_center(lbl_r);

  // Timer to update while running (20Hz)
  lv_timer_create(tick_cb, 50, NULL);

  return scr;
}
