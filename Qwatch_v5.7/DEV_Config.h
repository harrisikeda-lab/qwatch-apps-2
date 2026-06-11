#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "config.h"

static inline void DEV_I2C_EnsureInit() {
  static bool s_i2c_ready = false;
  if (!s_i2c_ready) {
    Wire.begin(TOUCH_I2C_SDA, TOUCH_I2C_SCL);
    Wire.setClock(400000);
    s_i2c_ready = true;
  }
}

static inline void DEV_I2C_Write_Byte(uint8_t addr, uint8_t reg, uint8_t value) {
  DEV_I2C_EnsureInit();
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission(true);
}

static inline void DEV_I2C_Read_nByte(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len) {
  DEV_I2C_EnsureInit();
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom((int)addr, (int)len, (int)true);

  uint16_t i = 0;
  while (Wire.available() && i < len) {
    buf[i++] = Wire.read();
  }
  while (i < len) {
    buf[i++] = 0;
  }
}
