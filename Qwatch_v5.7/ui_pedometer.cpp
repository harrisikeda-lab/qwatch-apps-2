#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <FFat.h>

#include "ui_pedometer.h"
#include "app_manager.h"
#include "pedometer.h"

namespace {

constexpr const char* DAILY_STEPS_FILE = "/daily_steps_total.csv";

struct DailyEntry {
  char date[16];
  uint32_t steps;
};

static lv_obj_t* label_today_steps = nullptr;
static lv_obj_t* label_chart_scale_top = nullptr;
static lv_obj_t* label_chart_scale_mid = nullptr;
static lv_obj_t* label_chart_scale_bottom = nullptr;
static lv_obj_t* label_history_hint = nullptr;
static lv_obj_t* chart = nullptr;
static lv_chart_series_t* chart_series = nullptr;
static lv_obj_t* day_labels[7] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
static lv_obj_t* g_pedometer_screen = nullptr;

int weekday_index_from_date(const char* ymd) {
  if (!ymd || strlen(ymd) != 10) return -1;

  int year = 0;
  int month = 0;
  int day = 0;
  if (sscanf(ymd, "%d-%d-%d", &year, &month, &day) != 3) {
    return -1;
  }

  if (month < 1 || month > 12 || day < 1 || day > 31) {
    return -1;
  }

  // Sakamoto algorithm: 0=Sun,1=Mon,...6=Sat
  static const int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
  int y = year;
  if (month < 3) y -= 1;
  int w = (y + y / 4 - y / 100 + y / 400 + t[month - 1] + day) % 7;

  // Convert to Monday-first: Mon=0 ... Sun=6
  return (w + 6) % 7;
}

const char* weekday_letter_from_index(int idx) {
  static const char* kLetters[7] = {"M", "T", "W", "T", "F", "S", "S"};
  if (idx < 0 || idx > 6) return "";
  return kLetters[idx];
}

bool parse_daily_line(const String& line_in, DailyEntry& out) {
  String line = line_in;
  line.trim();
  if (line.length() == 0) return false;

  int comma = line.indexOf(',');
  if (comma <= 0) return false;

  String date = line.substring(0, comma);
  String steps = line.substring(comma + 1);
  date.trim();
  steps.trim();

  if (date.length() == 0 || steps.length() == 0) return false;

  strlcpy(out.date, date.c_str(), sizeof(out.date));
  out.steps = (uint32_t)steps.toInt();
  return true;
}

int load_last_7_entries(DailyEntry* out_entries, int max_entries) {
  if (max_entries < 7) return 0;

  for (int i = 0; i < 7; ++i) {
    out_entries[i].date[0] = '\0';
    out_entries[i].steps = 0;
  }

  File f = FFat.open(DAILY_STEPS_FILE, FILE_READ);
  if (!f) return 0;

  DailyEntry rolling[7];
  int count = 0;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    DailyEntry e;
    if (!parse_daily_line(line, e)) {
      continue;
    }

    if (count < 7) {
      rolling[count++] = e;
    } else {
      for (int i = 0; i < 6; ++i) {
        rolling[i] = rolling[i + 1];
      }
      rolling[6] = e;
    }
  }

  f.close();

  for (int i = 0; i < count; ++i) {
    out_entries[i] = rolling[i];
  }

  return count;
}

uint32_t compute_chart_max(const DailyEntry* entries, int count) {
  uint32_t max_steps = 0;
  for (int i = 0; i < count; ++i) {
    if (entries[i].steps > max_steps) {
      max_steps = entries[i].steps;
    }
  }

  if (max_steps < 4000) max_steps = 4000;

  const uint32_t round_to = 2000;
  max_steps = ((max_steps + round_to - 1) / round_to) * round_to;

  if (max_steps > 0) {
    max_steps += 1000;
  }

  return max_steps;
}

void build_chart_data(uint32_t* values_out, const char** labels_out, bool* present_out, int& actual_count_out) {
  for (int i = 0; i < 7; ++i) {
    values_out[i] = 0;
    labels_out[i] = "";
    present_out[i] = false;
  }

  DailyEntry entries[7];
  const int count = load_last_7_entries(entries, 7);
  actual_count_out = count;

  const int offset = 7 - count;  // right-align if fewer than 7 exist

  for (int i = 0; i < count; ++i) {
    const int slot = offset + i;
    values_out[slot] = entries[i].steps;
    present_out[slot] = true;

    const int wd = weekday_index_from_date(entries[i].date);
    labels_out[slot] = weekday_letter_from_index(wd);
  }
}

bool is_pedometer_screen_active() {
  return g_pedometer_screen && (lv_scr_act() == g_pedometer_screen);
}

void update_ui() {
  if (!label_today_steps || !chart || !chart_series ||
      !label_chart_scale_top || !label_chart_scale_mid ||
      !label_chart_scale_bottom || !label_history_hint) {
    return;
  }

  char buf[48];
  snprintf(buf, sizeof(buf), "Today: %lu steps", (unsigned long)pedometer_get_steps());
  lv_label_set_text(label_today_steps, buf);

  uint32_t values[7];
  const char* labels[7];
  bool present[7];
  int actual_count = 0;

  build_chart_data(values, labels, present, actual_count);

  DailyEntry entries[7];
  const int count = load_last_7_entries(entries, 7);
  const uint32_t chart_max = compute_chart_max(entries, count);

  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, (lv_coord_t)chart_max);

  snprintf(buf, sizeof(buf), "%lu", (unsigned long)chart_max);
  lv_label_set_text(label_chart_scale_top, buf);

  snprintf(buf, sizeof(buf), "%lu", (unsigned long)(chart_max / 2));
  lv_label_set_text(label_chart_scale_mid, buf);

  lv_label_set_text(label_chart_scale_bottom, "0");

  lv_chart_set_point_count(chart, 7);
  for (int i = 0; i < 7; ++i) {
    chart_series->y_points[i] = (lv_coord_t)values[i];
    lv_label_set_text(day_labels[i], labels[i]);

    lv_obj_set_style_text_color(
        day_labels[i],
        present[i] ? lv_color_white() : lv_color_make(90, 90, 90),
        0
    );

    // Centre each weekday label under its own chart column using the
    // actual rendered width of the glyph.
    lv_obj_update_layout(day_labels[i]);

    const lv_coord_t chart_x = lv_obj_get_x(chart);
    const lv_coord_t chart_y = lv_obj_get_y(chart);
    const lv_coord_t chart_w = lv_obj_get_width(chart);
    const lv_coord_t chart_h = lv_obj_get_height(chart);

    const lv_coord_t label_w = lv_obj_get_width(day_labels[i]);
    const lv_coord_t col_w = chart_w / 7;
    const lv_coord_t col_center_x = chart_x + (i * col_w) + (col_w / 2);
    const lv_coord_t label_x = col_center_x - (label_w / 2);
    const lv_coord_t label_y = chart_y + chart_h + 2;

    lv_obj_set_pos(day_labels[i], label_x, label_y);
  }
  lv_chart_refresh(chart);

  if (actual_count > 0) {
    lv_label_set_text(label_history_hint, "Last 7 days");
  } else {
    lv_label_set_text(label_history_hint, "No history yet");
  }
}

void ui_timer_cb(lv_timer_t* t) {
  (void)t;
  if (!is_pedometer_screen_active()) {
    return;
  }
  update_ui();
}

void on_screen_event(lv_event_t* e) {
  const lv_event_code_t code = lv_event_get_code(e);
  if (code == LV_EVENT_SCREEN_LOADED) {
    update_ui();
  }
}

void on_back_clicked(lv_event_t* e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
  app_manager_show_animated(APP_DRAWER, LV_SCR_LOAD_ANIM_MOVE_RIGHT);
}

void style_back_btn(lv_obj_t* btn) {
  lv_obj_set_size(btn, 42, 42);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btn, lv_color_make(20, 20, 20), 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(btn, 0, 0);
  lv_obj_set_style_shadow_width(btn, 0, 0);
}

} // namespace

lv_obj_t* ui_pedometer_create_screen() {
  lv_obj_t* scr = lv_obj_create(NULL);
  g_pedometer_screen = scr;
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_size(scr, 240, 240);
  lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(scr, 0, 0);
  lv_obj_set_style_radius(scr, 0, 0);
  lv_obj_add_event_cb(scr, on_screen_event, LV_EVENT_ALL, NULL);

  lv_obj_t* btn_back = lv_btn_create(scr);
  style_back_btn(btn_back);
  lv_obj_align(btn_back, LV_ALIGN_TOP_LEFT, 8, 8);
  lv_obj_add_event_cb(btn_back, on_back_clicked, LV_EVENT_CLICKED, NULL);

  lv_obj_t* lb = lv_label_create(btn_back);
  lv_label_set_text(lb, LV_SYMBOL_LEFT);
  lv_obj_center(lb);

  lv_obj_t* title = lv_label_create(scr);
  lv_label_set_text(title, "Steps");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  label_history_hint = lv_label_create(scr);
  lv_label_set_text(label_history_hint, "Last 7 days");
  lv_obj_set_style_text_font(label_history_hint, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label_history_hint, lv_color_make(150, 150, 150), 0);
  lv_obj_align(label_history_hint, LV_ALIGN_TOP_MID, 0, 32);

  chart = lv_chart_create(scr);
  lv_obj_set_size(chart, 160, 96);
  lv_obj_align(chart, LV_ALIGN_TOP_LEFT, 18, 52);
  lv_obj_set_style_bg_color(chart, lv_color_make(18, 18, 18), 0);
  lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(chart, 1, 0);
  lv_obj_set_style_border_color(chart, lv_color_make(60, 60, 60), 0);
  lv_obj_set_style_radius(chart, 0, 0);
  lv_obj_set_style_pad_all(chart, 4, 0);
  lv_obj_set_style_line_width(chart, 1, LV_PART_MAIN);
  lv_obj_set_style_line_color(chart, lv_color_make(55, 55, 55), LV_PART_MAIN);
  lv_obj_set_style_line_opa(chart, LV_OPA_40, LV_PART_MAIN);
  lv_obj_clear_flag(chart, LV_OBJ_FLAG_SCROLLABLE);
  lv_chart_set_type(chart, LV_CHART_TYPE_BAR);
  lv_chart_set_div_line_count(chart, 3, 7);
  lv_chart_set_point_count(chart, 7);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 0, 12000);

  chart_series = lv_chart_add_series(
      chart,
      lv_color_make(255, 140, 0),
      LV_CHART_AXIS_PRIMARY_Y
  );

  // Make bars look like smartwatch activity bars
  lv_obj_set_style_radius(chart, 8, LV_PART_ITEMS);
  lv_obj_set_style_bg_opa(chart, LV_OPA_COVER, LV_PART_ITEMS);
  lv_obj_set_style_pad_column(chart, 4, LV_PART_MAIN);
  lv_obj_set_style_shadow_width(chart, 8, LV_PART_ITEMS);
  lv_obj_set_style_shadow_color(chart, lv_color_make(255, 140, 0), LV_PART_ITEMS);
  lv_obj_set_style_shadow_opa(chart, LV_OPA_30, LV_PART_ITEMS);

  for (int i = 0; i < 7; ++i) {
    day_labels[i] = lv_label_create(scr);
    lv_label_set_text(day_labels[i], "");
    lv_obj_set_style_text_font(day_labels[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(day_labels[i], lv_color_make(200, 200, 200), 0);
    lv_obj_set_pos(day_labels[i], 0, 0); // real placement happens in update_ui()
  }

  label_chart_scale_top = lv_label_create(scr);
  lv_label_set_text(label_chart_scale_top, "12000");
  lv_obj_set_style_text_font(label_chart_scale_top, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label_chart_scale_top, lv_color_make(180, 180, 180), 0);
  lv_obj_align_to(label_chart_scale_top, chart, LV_ALIGN_OUT_RIGHT_TOP, 6, -1);

  label_chart_scale_mid = lv_label_create(scr);
  lv_label_set_text(label_chart_scale_mid, "6000");
  lv_obj_set_style_text_font(label_chart_scale_mid, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label_chart_scale_mid, lv_color_make(180, 180, 180), 0);
  lv_obj_align_to(label_chart_scale_mid, chart, LV_ALIGN_OUT_RIGHT_MID, 6, 0);

  label_chart_scale_bottom = lv_label_create(scr);
  lv_label_set_text(label_chart_scale_bottom, "0");
  lv_obj_set_style_text_font(label_chart_scale_bottom, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(label_chart_scale_bottom, lv_color_make(180, 180, 180), 0);
  lv_obj_align_to(label_chart_scale_bottom, chart, LV_ALIGN_OUT_RIGHT_BOTTOM, 6, 1);

  label_today_steps = lv_label_create(scr);
  char buf[48];
  snprintf(buf, sizeof(buf), "Today: %lu steps", (unsigned long)pedometer_get_steps());
  lv_label_set_text(label_today_steps, buf);
  lv_obj_set_style_text_font(label_today_steps, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(label_today_steps, lv_color_white(), 0);
  lv_obj_align(label_today_steps, LV_ALIGN_BOTTOM_MID, 0, -10);

  lv_timer_create(ui_timer_cb, 1000, NULL);
  if (is_pedometer_screen_active()) {
    update_ui();
  }

  return scr;
}
