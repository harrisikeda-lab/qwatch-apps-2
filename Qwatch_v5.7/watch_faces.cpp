#include "watch_faces.h"

#include "fonts.h"
#include "images.h"

namespace {

enum DigitalDateMode : uint8_t {
  DIGITAL_DATE_FORMATTED = 0,
  DIGITAL_DATE_DAY_MDAY = 1
};

constexpr uint16_t zoom_for_size(int source_px, int target_px) {
  return (uint16_t)((target_px * 256) / source_px);
}

const DigitalWatchFaceDef DIGITAL_FACES[] = {
  {
    "Neon Digital",
    &neon,
    240,
    zoom_for_size(240, 240),
    &Comfortaa_56,
    &lv_font_montserrat_22,
    0,
    1,
    DIGITAL_DATE_FORMATTED,
    0,
    0,
    0,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    0,
    -5,
    0,
    0,
    0,
    0,
    0,
    35,
    -18,
    68,
    20,
    68,
    -48,
    -56,
    22,
    -56,
    48,
    -56,
    lv_color_white(),
    lv_color_make(180, 180, 180),
    LV_TEXT_ALIGN_LEFT,
    1,
    1,
    1,
    1,
    1,
    1
  },
  {
    "Chronos Digital",
    &chronos_digital_face,
    240,
    zoom_for_size(240, 240),
    &orbitron,
    &orbitron_24,
    1,
    0,
    DIGITAL_DATE_DAY_MDAY,
    1,
    1,
    1,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_CENTER,
    LV_ALIGN_TOP_MID,
    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_TOP_RIGHT,
    LV_ALIGN_TOP_RIGHT,
    0,
    0,
    -37,
    -25,
    -40,
    40,
    0,
    19,
    -26,
    87,
    0,
    0,
    -62,
    94,
    -62,
    61,
    -32,
    61,
    lv_color_make(255, 220, 0),
    lv_color_make(255, 220, 0),
    LV_TEXT_ALIGN_CENTER,
    1,
    1,
    0,
    1,
    1,
    1
  },
  {
    "Brix Digital",
    &brix,
    240,                           // source_size_px
    zoom_for_size(240, 240),       // main_zoom
    &biome_80,                     // time_font
    &lv_font_montserrat_24,        // date_font
    0,                             // split_time
    0,                             // show_seconds
    DIGITAL_DATE_DAY_MDAY,         // date_mode
    0,                             // show_date_box
    0,                             // hide_weather_temp
    0,                             // show_rings
    LV_ALIGN_CENTER,               // time_align
    LV_ALIGN_CENTER,               // hour_align
    LV_ALIGN_CENTER,               // minute_align
    LV_ALIGN_TOP_MID,              // date_align
    LV_ALIGN_CENTER,               // weather_icon_align
    LV_ALIGN_CENTER,               // weather_temp_align
    LV_ALIGN_CENTER,               // battery_align
    LV_ALIGN_CENTER,               // wifi_align
    LV_ALIGN_CENTER,               // bt_align
    0,                             // time_x
    0,                             // time_y
    0,                             // hour_x
    0,                             // hour_y
    0,                             // minute_x
    0,                             // minute_y
    0,                             // date_x
    12,                            // date_y
    -23,                           // weather_icon_x
    68,                            // weather_icon_y
    25,                            // weather_temp_x
    68,                            // weather_temp_y
    -48,                           // battery_x
    -56,                           // battery_y
    47,                            // wifi_x
    -56,                           // wifi_y
    73,                            // bt_x
    -56,                           // bt_y
    lv_color_white(),              // time_color
    lv_color_make(255, 255, 255),  // date_color
    LV_TEXT_ALIGN_CENTER,          // date_text_align
    1,                             // show_date
    1,                             // show_weather_icon
    1,                             // show_weather_temp
    1,                             // show_battery
    1,                             // show_wifi
    1                              // show_bt
  },
  {
  "Orbit Digital",
  &orbit_digital,
  240,                           // source_size_px
  zoom_for_size(240, 240),       // main_zoom
  &orbitron,                     // time_font (60px Orbitron)
  &lv_font_montserrat_24,        // date_font
  0,                             // split_time
  0,                             // show_seconds  <-- HH:MM ONLY
  DIGITAL_DATE_DAY_MDAY,         // date_mode
  0,                             // show_date_box
  0,                             // hide_weather_temp
  0,                             // show_rings
  LV_ALIGN_CENTER,               // time_align
  LV_ALIGN_CENTER,               // hour_align
  LV_ALIGN_CENTER,               // minute_align
  LV_ALIGN_TOP_MID,              // date_align
  LV_ALIGN_CENTER,               // weather_icon_align
  LV_ALIGN_CENTER,               // weather_temp_align
  LV_ALIGN_CENTER,               // battery_align
  LV_ALIGN_CENTER,               // wifi_align
  LV_ALIGN_CENTER,               // bt_align
  0,                             // time_x
  0,                             // time_y
  0,                             // hour_x
  0,                             // hour_y
  0,                             // minute_x
  0,                             // minute_y
  0,                             // date_x
  12,                            // date_y
  -23,                           // weather_icon_x
  68,                            // weather_icon_y
  25,                            // weather_temp_x
  68,                            // weather_temp_y
  -48,                           // battery_x
  -56,                           // battery_y
  47,                            // wifi_x
  -56,                           // wifi_y
  73,                            // bt_x
  -56,                           // bt_y
  lv_color_white(),              // time_color
  lv_color_make(255, 255, 255),  // date_color
  LV_TEXT_ALIGN_CENTER,          // date_text_align
  1,                             // show_date
  1,                             // show_weather_icon
  1,                             // show_weather_temp
  1,                             // show_battery
  1,                             // show_wifi
  1                              // show_bt
},

  {
  "Circuit Digital",
  &circuit,
  240,                           // source_size_px
  zoom_for_size(240, 240),       // main_zoom
  &orbitron,                     // time_font (60px Orbitron)
  &lv_font_montserrat_24,        // date_font
  0,                             // split_time
  0,                             // show_seconds  <-- HH:MM ONLY
  DIGITAL_DATE_DAY_MDAY,         // date_mode
  0,                             // show_date_box
  0,                             // hide_weather_temp
  0,                             // show_rings
  LV_ALIGN_CENTER,               // time_align
  LV_ALIGN_CENTER,               // hour_align
  LV_ALIGN_CENTER,               // minute_align
  LV_ALIGN_TOP_MID,              // date_align
  LV_ALIGN_CENTER,               // weather_icon_align
  LV_ALIGN_CENTER,               // weather_temp_align
  LV_ALIGN_CENTER,               // battery_align
  LV_ALIGN_CENTER,               // wifi_align
  LV_ALIGN_CENTER,               // bt_align
  0,                             // time_x
  0,                             // time_y
  0,                             // hour_x
  0,                             // hour_y
  0,                             // minute_x
  0,                             // minute_y
  0,                             // date_x
  12,                            // date_y
  -23,                           // weather_icon_x
  68,                            // weather_icon_y
  25,                            // weather_temp_x
  68,                            // weather_temp_y
  -48,                           // battery_x
  -56,                           // battery_y
  47,                            // wifi_x
  -56,                           // wifi_y
  73,                            // bt_x
  -56,                           // bt_y
  lv_color_white(),              // time_color
  lv_color_make(255, 255, 255),  // date_color
  LV_TEXT_ALIGN_CENTER,          // date_text_align
  1,                             // show_date
  1,                             // show_weather_icon
  1,                             // show_weather_temp
  1,                             // show_battery
  1,                             // show_wifi
  1                              // show_bt
},
    {
    "Stone Digital",
    &stone,
    240,                           // source_size_px
    zoom_for_size(240, 240),       // main_zoom
    &biome_80,                     // time_font
    &lv_font_montserrat_24,        // date_font
    0,                             // split_time
    0,                             // show_seconds
    DIGITAL_DATE_DAY_MDAY,         // date_mode
    0,                             // show_date_box
    1,                             // hide_weather_temp
    0,                             // show_rings
    LV_ALIGN_CENTER,               // time_align
    LV_ALIGN_CENTER,               // hour_align
    LV_ALIGN_CENTER,               // minute_align
    LV_ALIGN_TOP_MID,              // date_align
    LV_ALIGN_CENTER,               // weather_icon_align
    LV_ALIGN_CENTER,               // weather_temp_align
    LV_ALIGN_CENTER,               // battery_align
    LV_ALIGN_CENTER,               // wifi_align
    LV_ALIGN_CENTER,               // bt_align
    0,                             // time_x
    0,                             // time_y
    0,                             // hour_x
    0,                             // hour_y
    0,                             // minute_x
    0,                             // minute_y
    0,                             // date_x
    12,                            // date_y
    -23,                           // weather_icon_x
    68,                            // weather_icon_y
    25,                            // weather_temp_x
    68,                            // weather_temp_y
    -48,                           // battery_x
    -56,                           // battery_y
    47,                            // wifi_x
    -56,                           // wifi_y
    73,                            // bt_x
    -56,                           // bt_y
    lv_color_make(255,255,255),    // time_color
    lv_color_make(255,255,255),    // date_color
    LV_TEXT_ALIGN_CENTER,          // date_text_align
    1,                             // show_date
    0,                             // show_weather_icon
    0,                             // show_weather_temp
    0,                             // show_battery
    0,                             // show_wifi
    0                              // show_bt
  },
      {
    "Feather Digital",
    &feather_digital,
    240,                           // source_size_px
    zoom_for_size(240, 240),       // main_zoom
    &biome_80,                     // time_font
    &lv_font_montserrat_24,        // date_font
    0,                             // split_time
    0,                             // show_seconds
    DIGITAL_DATE_DAY_MDAY,         // date_mode
    0,                             // show_date_box
    1,                             // hide_weather_temp
    0,                             // show_rings
    LV_ALIGN_CENTER,               // time_align
    LV_ALIGN_CENTER,               // hour_align
    LV_ALIGN_CENTER,               // minute_align
    LV_ALIGN_TOP_MID,              // date_align
    LV_ALIGN_CENTER,               // weather_icon_align
    LV_ALIGN_CENTER,               // weather_temp_align
    LV_ALIGN_CENTER,               // battery_align
    LV_ALIGN_CENTER,               // wifi_align
    LV_ALIGN_CENTER,               // bt_align
    0,                             // time_x
    0,                             // time_y
    0,                             // hour_x
    0,                             // hour_y
    0,                             // minute_x
    0,                             // minute_y
    0,                             // date_x
    12,                            // date_y
    -23,                           // weather_icon_x
    68,                            // weather_icon_y
    25,                            // weather_temp_x
    68,                            // weather_temp_y
    -48,                           // battery_x
    -56,                           // battery_y
    47,                            // wifi_x
    -56,                           // wifi_y
    73,                            // bt_x
    -56,                           // bt_y
    lv_color_make(255,255,255),    // time_color
    lv_color_make(255,255,255),    // date_color
    LV_TEXT_ALIGN_CENTER,          // date_text_align
    1,                             // show_date
    0,                             // show_weather_icon
    0,                             // show_weather_temp
    0,                             // show_battery
    0,                             // show_wifi
    0                              // show_bt
  },

      {
    "Puppy",
    &puppy,
    240,                           // source_size_px
    zoom_for_size(240, 240),       // main_zoom
    &biome_80,                     // time_font
    &lv_font_montserrat_24,        // date_font
    0,                             // split_time
    0,                             // show_seconds
    DIGITAL_DATE_DAY_MDAY,         // date_mode
    0,                             // show_date_box
    1,                             // hide_weather_temp
    0,                             // show_rings
    LV_ALIGN_CENTER,               // time_align
    LV_ALIGN_CENTER,               // hour_align
    LV_ALIGN_CENTER,               // minute_align
    LV_ALIGN_TOP_MID,              // date_align
    LV_ALIGN_CENTER,               // weather_icon_align
    LV_ALIGN_CENTER,               // weather_temp_align
    LV_ALIGN_CENTER,               // battery_align
    LV_ALIGN_CENTER,               // wifi_align
    LV_ALIGN_CENTER,               // bt_align
    0,                             // time_x
    0,                             // time_y
    0,                             // hour_x
    0,                             // hour_y
    0,                             // minute_x
    0,                             // minute_y
    0,                             // date_x
    12,                            // date_y
    -23,                           // weather_icon_x
    68,                            // weather_icon_y
    25,                            // weather_temp_x
    68,                            // weather_temp_y
    -48,                           // battery_x
    -56,                           // battery_y
    47,                            // wifi_x
    -56,                           // wifi_y
    73,                            // bt_x
    -56,                           // bt_y
    lv_color_make(255,255,255),    // time_color
    lv_color_make(255,255,255),    // date_color
    LV_TEXT_ALIGN_CENTER,          // date_text_align
    1,                             // show_date
    0,                             // show_weather_icon
    0,                             // show_weather_temp
    0,                             // show_battery
    0,                             // show_wifi
    0                              // show_bt
  }
};

const AnalogueWatchFaceDef ANALOGUE_FACES[] = {
  {
    "Feather",
    &feather
    ,
    5,
    3,
    2,
    46,
    66,
    78,
    10,
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    0,
    0,
    -74,
    0
  },

   {
    "Puppy",
    &puppy
    ,
    5,
    3,
    2,
    46,
    66,
    78,
    10,
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    0,
    0,
    -74,
    0
  }, 
    {
    "Neon Analogue",
    &neon,
    5,
    3,
    2,
    46,
    66,
    78,
    10,
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    0,
    0,
    -74,
    0
    },
  {
    "Orbit",
    &orbit,
    6,
    6,
    2,
    60,
    80,
    85,
    10,
    lv_color_make(255, 255, 0),
    lv_color_make(255, 255, 0),
    lv_color_make(255, 0, 0),
    lv_color_make(255, 255, 0),
    0,
    0,
    -74,
    0
  },

  {
    "Circuit",
    &circuit,
    6,
    6,
    2,
    60,
    80,
    85,
    10,
    lv_color_make(255, 255, 0),
    lv_color_make(255, 255, 0),
    lv_color_make(255, 0, 0),
    lv_color_make(255, 255, 0),
    0,
    0,
    -74,
    0
  },

  {
    "Brix",
    &brix,                        // image
    6,                             // hour_width
    4,                             // minute_width
    3,                             // second_width
    46,                            // hour_length
    66,                            // minute_length
    78,                            // second_length
    10,                            // centre_dot_size
    lv_color_white(),              // hour_color
    lv_color_white(),              // minute_color
    lv_color_white(),              // second_color
    lv_color_white(),              // centre_dot_color
    0,                             // line_rounded
    1,                             // show_date
    -86,                           // date_y
    0                              // animate_hands
  },
  {
    "Stone",
    &stone,
    5,
    3,
    2,
    46,
    66,
    78,
    10,
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    lv_color_white(),
    0,
    0,
    -74,
    0
  },
  {
    "Chronos",
    &chronos,
    9,
    7,
    2,
    46,
    66,
    78,
    10,
    lv_color_make(255, 210, 40),
    lv_color_make(255, 210, 40),
    lv_color_make(255, 40, 40),
    lv_color_make(255, 210, 40),
    1,
    1,
    -72,
    1
  }
};

int clamp_index(int index, int count) {
  if (count <= 0) return 0;
  if (index < 0) return 0;
  if (index >= count) return count - 1;
  return index;
}

}

int watch_faces_get_digital_count() {
  return (int)(sizeof(DIGITAL_FACES) / sizeof(DIGITAL_FACES[0]));
}

int watch_faces_get_analogue_count() {
  return (int)(sizeof(ANALOGUE_FACES) / sizeof(ANALOGUE_FACES[0]));
}

const DigitalWatchFaceDef* watch_faces_get_digital(int index) {
  const int count = watch_faces_get_digital_count();
  if (count <= 0) return nullptr;
  return &DIGITAL_FACES[clamp_index(index, count)];
}

const AnalogueWatchFaceDef* watch_faces_get_analogue(int index) {
  const int count = watch_faces_get_analogue_count();
  if (count <= 0) return nullptr;
  return &ANALOGUE_FACES[clamp_index(index, count)];
}

const lv_img_dsc_t* watch_faces_get_digital_preview_image(int index) {
  const DigitalWatchFaceDef* face = watch_faces_get_digital(index);
  return face ? face->image : nullptr;
}

const lv_img_dsc_t* watch_faces_get_analogue_preview_image(int index) {
  const AnalogueWatchFaceDef* face = watch_faces_get_analogue(index);
  return face ? face->image : nullptr;
}

uint16_t watch_faces_get_digital_preview_zoom(int index, int target_px) {
  (void)index;
  return zoom_for_size(240, target_px);
}

uint16_t watch_faces_get_analogue_preview_zoom(int index, int target_px) {
  (void)index;
  return zoom_for_size(240, target_px);
}
