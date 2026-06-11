#pragma once
#include <lvgl.h>

enum AppId {
  APP_WATCHFACE = 0,
  APP_DRAWER    = 1,
  APP_STOPWATCH = 2,
  APP_SETTINGS  = 3,
  APP_PEDOMETER = 4
};

void app_manager_init();
void app_manager_show(AppId id);
void app_manager_show_animated(AppId id, lv_scr_load_anim_t anim);

// Called from touch driver when a gesture is detected by the touch controller.
void app_manager_on_swipe_up();
void app_manager_on_swipe_down();
void app_manager_on_swipe_left();
void app_manager_on_swipe_right();
