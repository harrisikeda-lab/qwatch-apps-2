#pragma once

// ---- WiFi credentials ----
// Leave QWATCH_WIFI_SSID empty to use credentials entered through the
// Wi-Fi settings page and loaded from FFat.
//
// If QWATCH_WIFI_SSID is set here, these credentials are copied into the
// normal qwatch_settings Wi-Fi storage at boot using the same CSV format
// as the LVGL keyboard Wi-Fi settings page.
#define QWATCH_WIFI_SSID     ""
#define QWATCH_WIFI_PASSWORD ""

// ---- Time / NTP ----
// The local UTC offset is detected automatically from the hourly
// Open-Meteo weather response. No manual timezone setting is required.

// NTP servers (you can leave these as-is)
#define NTP_SERVER_1 "pool.ntp.org"
#define NTP_SERVER_2 "time.google.com"
#define NTP_SERVER_3 "time.cloudflare.com"

// Daily scheduled NTP sync time (local time, 24-hour clock).
// After the initial boot/first-sync behaviour, the watch will only
// attempt one scheduled NTP re-sync per day at this time.
#define NTP_SYNC_HOUR   3
#define NTP_SYNC_MINUTE 0
#define NTP_SYNC_SECOND 0

// ---- Battery ADC ----
// Waveshare ESP32-S3-Touch-LCD-1.28 routes VBAT to GPIO1 via a 200K/100K divider.
#define BAT_ADC_PIN 1
#define BATTERY_ADC_SAMPLES 8

// ---- Touch (CST816S) ----
// Waveshare ESP32-S3-Touch-LCD-1.28 commonly uses:
// I2C: SDA=6, SCL=7; Touch: RST=13, IRQ=5
#define TOUCH_I2C_SDA 6
#define TOUCH_I2C_SCL 7
#define TOUCH_RST 13
#define TOUCH_IRQ 5

// ---- IMU wake interrupt ----
// Board documentation maps QMI8658 INT2 to GPIO3.
#define IMU_INT1 3

// ---- LCD backlight ----
// Waveshare ESP32-S3-Touch-LCD-1.28 uses GPIO2 for LCD_BL.
#define LCD_BL 2
