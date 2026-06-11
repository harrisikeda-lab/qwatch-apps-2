#pragma once

#include <stdbool.h>
#include <stdint.h>

void weather_service_init();
void weather_service_loop();
bool weather_service_refresh_now();

bool weather_service_is_valid();
int weather_service_get_temperature_c();
int32_t weather_service_get_utc_offset_seconds();
const char* weather_service_get_icon_utf8();
