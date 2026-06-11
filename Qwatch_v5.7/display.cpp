#include "display.h"

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Arduino.h>

#include "config.h"

static TFT_eSPI tft = TFT_eSPI();

static lv_color_t draw_buf_1[240 * 20];
static lv_disp_draw_buf_t draw_buf;
static lv_obj_t* dim_overlay = nullptr;
static bool g_sleeping = false;

static void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  if (g_sleeping) {
    lv_disp_flush_ready(disp);
    return;
  }

  const uint32_t w = (area->x2 - area->x1 + 1);
  const uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, false);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

static void ensure_dim_overlay() {
  if (dim_overlay) {
    return;
  }

  dim_overlay = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(dim_overlay);
  lv_obj_set_size(dim_overlay, 240, 240);
  lv_obj_align(dim_overlay, LV_ALIGN_CENTER, 0, 0);
  lv_obj_clear_flag(dim_overlay, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(dim_overlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(dim_overlay, LV_OPA_TRANSP, 0);
  lv_obj_add_flag(dim_overlay, LV_OBJ_FLAG_HIDDEN);
}

static void set_backlight_enabled(bool enabled) {
  digitalWrite(LCD_BL, enabled ? HIGH : LOW);
}

void display_init() {
  pinMode(LCD_BL, OUTPUT);
  set_backlight_enabled(true);

  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  lv_disp_draw_buf_init(&draw_buf, draw_buf_1, NULL, 240 * 20);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 240;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  disp_drv.sw_rotate = 1;

  lv_disp_t* disp = lv_disp_drv_register(&disp_drv);
  lv_disp_set_rotation(disp, LV_DISP_ROT_90);

  ensure_dim_overlay();
}

void display_set_dimmed(bool dimmed) {
  ensure_dim_overlay();

  if (g_sleeping) {
    return;
  }

  if (dimmed) {
    lv_obj_clear_flag(dim_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dim_overlay);
    lv_obj_set_style_bg_opa(dim_overlay, LV_OPA_50, 0);
  } else {
    lv_obj_set_style_bg_opa(dim_overlay, LV_OPA_TRANSP, 0);
    lv_obj_add_flag(dim_overlay, LV_OBJ_FLAG_HIDDEN);
  }
}

void display_set_sleep(bool sleeping) {
  ensure_dim_overlay();

  if (sleeping == g_sleeping) {
    return;
  }

  if (sleeping) {
    lv_obj_clear_flag(dim_overlay, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(dim_overlay);
    lv_obj_set_style_bg_opa(dim_overlay, LV_OPA_COVER, 0);

    set_backlight_enabled(false);

    tft.writecommand(0x28);  // display off
    delay(10);
    tft.writecommand(0x10);  // sleep in
    delay(120);

    g_sleeping = true;
    return;
  }

  tft.writecommand(0x11);  // sleep out
  delay(120);

  g_sleeping = false;
  lv_obj_set_style_bg_opa(dim_overlay, LV_OPA_TRANSP, 0);
  lv_obj_add_flag(dim_overlay, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(lv_scr_act());
  lv_refr_now(lv_disp_get_default());

  tft.writecommand(0x29);  // display on
  delay(20);

  set_backlight_enabled(true);
}

bool display_is_sleeping() {
  return g_sleeping;
}
