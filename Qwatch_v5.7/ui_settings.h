#pragma once
#include <lvgl.h>

lv_obj_t* ui_settings_create_screen();
bool ui_settings_handle_swipe_up();
bool ui_settings_handle_swipe_down();
bool ui_settings_handle_swipe_left();
bool ui_settings_handle_swipe_right();
