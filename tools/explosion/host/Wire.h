#pragma once
#include <stdint.h>
struct TwoWire {
  void begin() {} void end() {} void setClock(uint32_t) {}
  void beginTransmission(int) {} uint8_t endTransmission() { return 0; }
  void write(uint8_t) {} int read() { return 0; } void requestFrom(int, int) {}
};
static TwoWire Wire;
