#pragma once
#include <lvgl.h>

// Create and return a dedicated LVGL screen for the watch face.
lv_obj_t* ui_watch_create_screen();
void ui_watch_refresh_now();
void ui_watch_pause_timer();
void ui_watch_resume_timer();
