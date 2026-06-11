#include "pedometer.h"

#include <Arduino.h>
#include <math.h>
#include <time.h>
#include <FFat.h>

#include "QMI8658.h"
#include "qwatch_settings.h"

namespace {

constexpr uint32_t SAMPLE_INTERVAL_MS = 20;          // 50 Hz
constexpr uint32_t MIN_STEP_INTERVAL_MS = 250;       // reject double counts
constexpr uint32_t MAX_STEP_INTERVAL_MS = 900;       // plausible walking cadence timeout
constexpr uint8_t  MIN_CONSECUTIVE_PEAKS = 3;        // confirm walking pattern

// QMI8658 accel appears to be in mg units (1000 ~= 1g)
constexpr float    BASE_STEP_THRESHOLD = 80.0f;      // ~0.08g filtered peak
constexpr float    DYN_THRESHOLD = 40.0f;            // ~0.04g raw dynamic movement gate
constexpr float    GRAVITY_ALPHA = 0.92f;            // slow baseline tracking
constexpr float    SIGNAL_ALPHA = 0.15f;             // heavier smoothing

constexpr const char* CURRENT_STEPS_FILE = "/current_steps_total.csv";
constexpr const char* DAILY_STEPS_FILE   = "/daily_steps_total.csv";

bool g_running = false;
uint32_t g_steps = 0;
uint32_t g_last_sample_ms = 0;
uint32_t g_last_peak_ms = 0;
uint8_t g_consecutive_peaks = 0;

float g_baseline_mag = 1000.0f;
float g_filtered_signal = 0.0f;
float g_prev_filtered_signal = 0.0f;
float g_last_magnitude = 1000.0f;

bool g_fs_ready = false;

// Guards so scheduled jobs run once only
int g_last_snapshot_year = -1;
int g_last_snapshot_yday = -1;
int g_last_snapshot_hour = -1;

int g_last_daily_append_year = -1;
int g_last_daily_append_yday = -1;
void reset_sequence_state() {
  g_consecutive_peaks = 0;
  g_last_peak_ms = 0;
}

float current_threshold() {
  return BASE_STEP_THRESHOLD;
}
void process_sample(uint32_t now) {
  float acc[3] = {0.0f, 0.0f, 0.0f};
  float gyro[3] = {0.0f, 0.0f, 0.0f};
  unsigned int tim_count = 0;
  QMI8658_read_xyz(acc, gyro, &tim_count);

  const float mag = sqrtf((acc[0] * acc[0]) + (acc[1] * acc[1]) + (acc[2] * acc[2]));
  g_last_magnitude = mag;

  // Track gravity / long-term magnitude baseline
  g_baseline_mag = (GRAVITY_ALPHA * g_baseline_mag) + ((1.0f - GRAVITY_ALPHA) * mag);
  const float dyn = mag - g_baseline_mag;

  // Smooth dynamic signal more heavily to suppress hand jitter
  g_prev_filtered_signal = g_filtered_signal;
  g_filtered_signal = (SIGNAL_ALPHA * dyn) + ((1.0f - SIGNAL_ALPHA) * g_filtered_signal);

  const float threshold = current_threshold();
  bool peak = false;
  bool step = false;

  if (g_last_peak_ms != 0 && (now - g_last_peak_ms) > MAX_STEP_INTERVAL_MS) {
    g_consecutive_peaks = 0;
  }

  // Require:
  // 1) enough real dynamic movement
  // 2) filtered signal crossing upward through threshold
  if ((fabsf(dyn) > DYN_THRESHOLD) &&
      (g_filtered_signal > threshold) &&
      (g_prev_filtered_signal <= threshold)) {
    peak = true;

    // Enforce minimum step interval
    const bool interval_ok =
        (g_last_peak_ms == 0) || ((now - g_last_peak_ms) >= MIN_STEP_INTERVAL_MS);

    if (interval_ok) {
      if (g_last_peak_ms == 0 || (now - g_last_peak_ms) <= MAX_STEP_INTERVAL_MS) {
        if (g_consecutive_peaks < 255) {
          ++g_consecutive_peaks;
        }
      } else {
        g_consecutive_peaks = 1;
      }

      g_last_peak_ms = now;

      if (g_consecutive_peaks >= MIN_CONSECUTIVE_PEAKS) {
        ++g_steps;
        step = true;
      }
    }
  }

}

bool get_local_time_info(struct tm& out_tm) {
  time_t now = time(nullptr);
  if (now < 1700000000) {  // sanity check: RTC/NTP not ready
    return false;
  }
  localtime_r(&now, &out_tm);
  return true;
}

void format_date_ymd(const struct tm& tm_info, char* out, size_t out_len) {
  snprintf(out, out_len, "%04d-%02d-%02d",
           tm_info.tm_year + 1900,
           tm_info.tm_mon + 1,
           tm_info.tm_mday);
}

bool file_exists(const char* path) {
  File f = FFat.open(path, FILE_READ);
  if (!f) return false;
  f.close();
  return true;
}

void write_current_steps_snapshot_for_date(const char* date_ymd, uint32_t steps) {
  if (!g_fs_ready) return;

  File f = FFat.open(CURRENT_STEPS_FILE, FILE_WRITE);
  if (!f) return;

  f.seek(0);
  f.print(date_ymd);
  f.print(",");
  f.println((unsigned long)steps);
  f.close();
}

bool read_current_steps_snapshot(char* date_out, size_t date_out_len, uint32_t& steps_out) {
  if (!g_fs_ready) return false;

  File f = FFat.open(CURRENT_STEPS_FILE, FILE_READ);
  if (!f) return false;

  String line = f.readStringUntil('\n');
  f.close();
  line.trim();

  if (line.length() == 0) return false;

  int comma = line.indexOf(',');
  if (comma <= 0) return false;

  String date = line.substring(0, comma);
  String steps = line.substring(comma + 1);

  date.trim();
  steps.trim();

  if (date.length() == 0 || steps.length() == 0) return false;

  strlcpy(date_out, date.c_str(), date_out_len);
  steps_out = (uint32_t)steps.toInt();
  return true;
}

bool read_last_daily_date(char* date_out, size_t date_out_len) {
  if (!g_fs_ready) return false;

  File f = FFat.open(DAILY_STEPS_FILE, FILE_READ);
  if (!f) return false;

  String last_line;
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      last_line = line;
    }
  }
  f.close();

  if (last_line.length() == 0) return false;

  int comma = last_line.indexOf(',');
  if (comma <= 0) return false;

  String date = last_line.substring(0, comma);
  date.trim();
  if (date.length() == 0) return false;

  strlcpy(date_out, date.c_str(), date_out_len);
  return true;
}

void append_daily_steps_if_new(const char* date_ymd, uint32_t steps) {
  if (!g_fs_ready) return;

  char last_date[16] = {0};
  if (read_last_daily_date(last_date, sizeof(last_date))) {
    if (strcmp(last_date, date_ymd) == 0) {
      return; // already appended for this date
    }
  }

  File f = FFat.open(DAILY_STEPS_FILE, FILE_APPEND);
  if (!f) return;

  f.print(date_ymd);
  f.print(",");
  f.println((unsigned long)steps);
  f.close();
}

void create_daily_steps_file_if_missing() {
  if (!g_fs_ready) return;
  if (file_exists(DAILY_STEPS_FILE)) return;

  File f = FFat.open(DAILY_STEPS_FILE, FILE_WRITE);
  if (!f) return;
  f.close();
}

void create_initial_current_steps_file_if_missing() {
  if (!g_fs_ready) return;
  if (file_exists(CURRENT_STEPS_FILE)) return;

  struct tm now_tm = {};
  char date_buf[16] = "1970-01-01";
  if (get_local_time_info(now_tm)) {
    format_date_ymd(now_tm, date_buf, sizeof(date_buf));
  }

  write_current_steps_snapshot_for_date(date_buf, g_steps);
}

void sync_running_state_with_settings() {
  const bool want_active = qwatch_settings_get_steps_active();

  if (want_active && !g_running) {
    pedometer_start();
  } else if (!want_active && g_running) {
    pedometer_pause();
  }
}

void handle_scheduled_storage() {
  if (!g_fs_ready) return;

  struct tm now_tm = {};
  if (!get_local_time_info(now_tm)) {
    return;
  }

  char today_buf[16];
  format_date_ymd(now_tm, today_buf, sizeof(today_buf));

  // Overwrite current_steps_total.csv at 01:55, 03:55, 05:55 ... 23:55
  if ((now_tm.tm_min == 55) && ((now_tm.tm_hour % 2) == 1)) {
    if (!(g_last_snapshot_year == now_tm.tm_year &&
          g_last_snapshot_yday == now_tm.tm_yday &&
          g_last_snapshot_hour == now_tm.tm_hour)) {
      write_current_steps_snapshot_for_date(today_buf, g_steps);
      g_last_snapshot_year = now_tm.tm_year;
      g_last_snapshot_yday = now_tm.tm_yday;
      g_last_snapshot_hour = now_tm.tm_hour;
    }
  }

  // Append one daily total at 23:58 using the current_steps_total snapshot if available
  if (now_tm.tm_hour == 23 && now_tm.tm_min == 58) {
    if (!(g_last_daily_append_year == now_tm.tm_year &&
          g_last_daily_append_yday == now_tm.tm_yday)) {
      char snapshot_date[16] = {0};
      uint32_t snapshot_steps = g_steps;

      if (!read_current_steps_snapshot(snapshot_date, sizeof(snapshot_date), snapshot_steps)) {
        strlcpy(snapshot_date, today_buf, sizeof(snapshot_date));
      }

      append_daily_steps_if_new(snapshot_date, snapshot_steps);
      g_steps = 0;

      g_last_daily_append_year = now_tm.tm_year;
      g_last_daily_append_yday = now_tm.tm_yday;
    }
  }
}

} // namespace

void pedometer_init() {
  g_running = false;
  g_steps = 0;
  g_last_sample_ms = millis();
  g_baseline_mag = 1000.0f;
  g_filtered_signal = 0.0f;
  g_prev_filtered_signal = 0.0f;
  g_last_magnitude = 1000.0f;
  reset_sequence_state();

  g_fs_ready = FFat.begin(true);

  if (g_fs_ready) {
    create_daily_steps_file_if_missing();
    create_initial_current_steps_file_if_missing();
  }

  if (qwatch_settings_get_steps_active()) {
    pedometer_start();
  }
}

void pedometer_loop() {
  sync_running_state_with_settings();

  if (g_running) {
    const uint32_t now = millis();
    if ((now - g_last_sample_ms) >= SAMPLE_INTERVAL_MS) {
      g_last_sample_ms = now;
      process_sample(now);
    }
  }

  handle_scheduled_storage();
}

void pedometer_start() {
  if (g_running) {
    return;
  }

  g_running = true;
  g_last_sample_ms = millis();
  g_baseline_mag = g_last_magnitude;
  g_filtered_signal = 0.0f;
  g_prev_filtered_signal = 0.0f;
  reset_sequence_state();
}

void pedometer_pause() {
  g_running = false;
}
void pedometer_reset() {
  g_steps = 0;
  g_filtered_signal = 0.0f;
  g_prev_filtered_signal = 0.0f;
  g_baseline_mag = g_last_magnitude;
  reset_sequence_state();

  if (!g_fs_ready) {
    return;
  }

  struct tm now_tm = {};
  char date_buf[16] = "1970-01-01";
  if (get_local_time_info(now_tm)) {
    format_date_ymd(now_tm, date_buf, sizeof(date_buf));
  }

  write_current_steps_snapshot_for_date(date_buf, 0);

  FFat.remove(DAILY_STEPS_FILE);
  create_daily_steps_file_if_missing();
}
uint32_t pedometer_get_steps() {
  return g_steps;
}
