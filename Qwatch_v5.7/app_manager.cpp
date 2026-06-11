#include "app_manager.h"

#include "ui_watch.h"
#include "ui_drawer.h"
#include "ui_stopwatch.h"
#include "ui_settings.h"
#include "ui_pedometer.h"
#include "images.h"

static AppId current_app = APP_WATCHFACE;
static AppId pending_app = APP_WATCHFACE;

static lv_obj_t* scr_watch = nullptr;
static lv_obj_t* scr_drawer = nullptr;
static lv_obj_t* scr_stopwatch = nullptr;
static lv_obj_t* scr_settings = nullptr;
static lv_obj_t* scr_pedometer = nullptr;
static lv_obj_t* scr_boot = nullptr;
static lv_obj_t* boot_logo = nullptr;

static bool g_boot_sequence_active = false;
static bool g_transition_active = false;

static lv_timer_t* g_transition_timer = nullptr;
static bool g_watch_timer_paused_for_settings = false;

static constexpr uint32_t BOOT_FADE_MS = 1000;
static constexpr uint32_t BOOT_HOLD_MS = 1500;
static constexpr uint32_t BOOT_TOTAL_MS = (BOOT_FADE_MS * 2) + BOOT_HOLD_MS;

static constexpr uint32_t SCREEN_ANIM_TIME_MS = 220;
static constexpr uint32_t SCREEN_ANIM_DELAY_MS = 0;

static void boot_logo_set_opa(void* obj, int32_t v) {
  lv_obj_set_style_img_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
}

static void start_boot_logo_anim(lv_opa_t from, lv_opa_t to, uint32_t duration_ms) {
  if (!boot_logo) return;

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, boot_logo);
  lv_anim_set_exec_cb(&a, boot_logo_set_opa);
  lv_anim_set_values(&a, from, to);
  lv_anim_set_time(&a, duration_ms);
  lv_anim_start(&a);
}

static void boot_fade_out_timer_cb(lv_timer_t* timer) {
  (void)timer;
  start_boot_logo_anim(LV_OPA_COVER, LV_OPA_TRANSP, BOOT_FADE_MS);
}

static void update_watch_timer_for_app(AppId id) {
  if (id == APP_SETTINGS) {
    if (!g_watch_timer_paused_for_settings) {
      ui_watch_pause_timer();
      g_watch_timer_paused_for_settings = true;
    }
  } else if (g_watch_timer_paused_for_settings) {
    ui_watch_resume_timer();
    g_watch_timer_paused_for_settings = false;
  }
}

static void transition_finish_timer_cb(lv_timer_t* timer) {
  (void)timer;
  current_app = pending_app;
  update_watch_timer_for_app(current_app);
  g_transition_active = false;
  g_transition_timer = nullptr;
}

static void boot_finish_timer_cb(lv_timer_t* timer) {
  (void)timer;
  g_boot_sequence_active = false;
  app_manager_show(APP_WATCHFACE);
}

static lv_obj_t* create_boot_screen() {
  lv_obj_t* scr = lv_obj_create(nullptr);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_style_radius(scr, 0, 0);

  boot_logo = lv_img_create(scr);
  lv_img_set_src(boot_logo, &qwatch_logo);
  lv_obj_center(boot_logo);
  lv_obj_set_style_img_opa(boot_logo, LV_OPA_TRANSP, 0);

  return scr;
}

static lv_obj_t* get_screen_for_app(AppId id) {
  switch (id) {
    case APP_WATCHFACE: return scr_watch;
    case APP_DRAWER:    return scr_drawer;
    case APP_STOPWATCH: return scr_stopwatch;
    case APP_SETTINGS:  return scr_settings;
    case APP_PEDOMETER: return scr_pedometer;
    default:            return scr_watch;
  }
}

void app_manager_show(AppId id) {
  pending_app = id;
  current_app = id;
  g_transition_active = false;

  if (g_transition_timer) {
    lv_timer_del(g_transition_timer);
    g_transition_timer = nullptr;
  }

  lv_obj_t* target = get_screen_for_app(id);
  if (target && lv_scr_act() != target) {
    lv_scr_load(target);
  }

  update_watch_timer_for_app(current_app);
}

void app_manager_show_animated(AppId id, lv_scr_load_anim_t anim) {
  if (g_boot_sequence_active) return;
  if (g_transition_active) return;

  lv_obj_t* target = get_screen_for_app(id);
  if (!target) return;

  if (lv_scr_act() == target) {
    current_app = id;
    pending_app = id;
    update_watch_timer_for_app(current_app);
    return;
  }

  pending_app = id;
  if (id == APP_SETTINGS && !g_watch_timer_paused_for_settings) {
    ui_watch_pause_timer();
    g_watch_timer_paused_for_settings = true;
  }
  g_transition_active = true;

  if (g_transition_timer) {
    lv_timer_del(g_transition_timer);
    g_transition_timer = nullptr;
  }

  lv_scr_load_anim(target, anim, SCREEN_ANIM_TIME_MS, SCREEN_ANIM_DELAY_MS, false);

  g_transition_timer = lv_timer_create(
    transition_finish_timer_cb,
    SCREEN_ANIM_TIME_MS + SCREEN_ANIM_DELAY_MS + 10,
    nullptr
  );
  lv_timer_set_repeat_count(g_transition_timer, 1);
}

void app_manager_init() {
  // Create all application screens once.
  scr_watch = ui_watch_create_screen();
  scr_drawer = ui_drawer_create_screen();
  scr_stopwatch = ui_stopwatch_create_screen();
  scr_settings = ui_settings_create_screen();
  scr_pedometer = ui_pedometer_create_screen();

  // Create and show the boot splash before entering the watchface.
  scr_boot = create_boot_screen();
  g_boot_sequence_active = true;
  g_transition_active = false;
  current_app = APP_WATCHFACE;
  pending_app = APP_WATCHFACE;

  lv_scr_load(scr_boot);

  start_boot_logo_anim(LV_OPA_TRANSP, LV_OPA_COVER, BOOT_FADE_MS);

  lv_timer_t* fade_out_timer = lv_timer_create(boot_fade_out_timer_cb, BOOT_FADE_MS + BOOT_HOLD_MS, nullptr);
  lv_timer_set_repeat_count(fade_out_timer, 1);

  lv_timer_t* finish_timer = lv_timer_create(boot_finish_timer_cb, BOOT_TOTAL_MS, nullptr);
  lv_timer_set_repeat_count(finish_timer, 1);
}

void app_manager_on_swipe_up() {
  if (g_boot_sequence_active) return;
  if (g_transition_active) return;

  if (current_app == APP_WATCHFACE) {
    app_manager_show_animated(APP_DRAWER, LV_SCR_LOAD_ANIM_MOVE_TOP);
  } else if (current_app == APP_SETTINGS) {
    app_manager_show_animated(APP_WATCHFACE, LV_SCR_LOAD_ANIM_MOVE_TOP);
  }
}

void app_manager_on_swipe_down() {
  if (g_boot_sequence_active) return;
  if (g_transition_active) return;

  if (current_app == APP_DRAWER) {
    app_manager_show_animated(APP_WATCHFACE, LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
  } else if (current_app == APP_WATCHFACE) {
    app_manager_show_animated(APP_SETTINGS, LV_SCR_LOAD_ANIM_MOVE_BOTTOM);
  }
}

void app_manager_on_swipe_left() {
  if (g_boot_sequence_active) return;
  if (g_transition_active) return;

  if (current_app == APP_WATCHFACE) {
    app_manager_show_animated(APP_STOPWATCH, LV_SCR_LOAD_ANIM_MOVE_LEFT);
  } else if (current_app == APP_PEDOMETER) {
    app_manager_show_animated(APP_WATCHFACE, LV_SCR_LOAD_ANIM_MOVE_LEFT);
  }
}

void app_manager_on_swipe_right() {
  if (g_boot_sequence_active) return;
  if (g_transition_active) return;

  if (current_app == APP_WATCHFACE) {
    app_manager_show_animated(APP_PEDOMETER, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
  } else if (current_app == APP_STOPWATCH) {
    app_manager_show_animated(APP_WATCHFACE, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
  }
}
