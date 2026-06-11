#pragma once

#include <stdbool.h>
#include <stdint.h>

void pedometer_init();
void pedometer_loop();

void pedometer_start();
void pedometer_pause();
void pedometer_reset();

uint32_t pedometer_get_steps();

// Historical daily totals for charting.
struct PedometerDailyEntry {
  char date[11];      // YYYY-MM-DD
  uint32_t steps;
};

int pedometer_get_last_7_days(PedometerDailyEntry* out_entries, int max_entries);
uint32_t pedometer_get_chart_max_steps();
bool pedometer_history_is_ready();
