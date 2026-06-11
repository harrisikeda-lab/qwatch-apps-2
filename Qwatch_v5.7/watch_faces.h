#pragma once

#include <lvgl.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  const char* name;
  const lv_img_dsc_t* image;
  uint16_t source_size_px;
  uint16_t main_zoom;
  const lv_font_t* time_font;
  const lv_font_t* date_font;
  uint8_t split_time;
  uint8_t show_seconds;
  uint8_t date_mode;
  uint8_t show_date_box;
  uint8_t hide_weather_temp;
  uint8_t show_rings;
  lv_align_t time_align;
  lv_align_t hour_align;
  lv_align_t minute_align;
  lv_align_t date_align;
  lv_align_t weather_icon_align;
  lv_align_t weather_temp_align;
  lv_align_t battery_align;
  lv_align_t wifi_align;
  lv_align_t bt_align;
  int16_t time_x;
  int16_t time_y;
  int16_t hour_x;
  int16_t hour_y;
  int16_t minute_x;
  int16_t minute_y;
  int16_t date_x;
  int16_t date_y;
  int16_t weather_icon_x;
  int16_t weather_icon_y;
  int16_t weather_temp_x;
  int16_t weather_temp_y;
  int16_t battery_x;
  int16_t battery_y;
  int16_t wifi_x;
  int16_t wifi_y;
  int16_t bt_x;
  int16_t bt_y;
  lv_color_t time_color;
  lv_color_t date_color;
  lv_text_align_t date_text_align;
  uint8_t show_date;
  uint8_t show_weather_icon;
  uint8_t show_weather_temp;
  uint8_t show_battery;
  uint8_t show_wifi;
  uint8_t show_bt;
} DigitalWatchFaceDef;

typedef struct {
  const char* name;
  const lv_img_dsc_t* image;
  uint16_t hour_width;
  uint16_t minute_width;
  uint16_t second_width;
  int16_t hour_length;
  int16_t minute_length;
  int16_t second_length;
  uint16_t centre_dot_size;
  lv_color_t hour_color;
  lv_color_t minute_color;
  lv_color_t second_color;
  lv_color_t centre_dot_color;
  uint8_t line_rounded;
  uint8_t show_date;
  int16_t date_y;
  uint8_t animate_hands;
} AnalogueWatchFaceDef;

int watch_faces_get_digital_count();
int watch_faces_get_analogue_count();
const DigitalWatchFaceDef* watch_faces_get_digital(int index);
const AnalogueWatchFaceDef* watch_faces_get_analogue(int index);

const lv_img_dsc_t* watch_faces_get_digital_preview_image(int index);
const lv_img_dsc_t* watch_faces_get_analogue_preview_image(int index);
uint16_t watch_faces_get_digital_preview_zoom(int index, int target_px);
uint16_t watch_faces_get_analogue_preview_zoom(int index, int target_px);

#ifdef __cplusplus
}
#endif
