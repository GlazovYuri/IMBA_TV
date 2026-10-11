// I2C в эмулятор контроллера SSH1106 (tools/previews/harness.cpp)
#pragma once
#include <stdint.h>
#include <stddef.h>

class TwoWire
{
public:
  void begin() {}
  void end() {}
  void setClock(uint32_t) {}
  void beginTransmission(uint8_t);
  size_t write(uint8_t b);
  uint8_t endTransmission(bool stop = true);
  uint8_t requestFrom(uint8_t, uint8_t) { return 0; }
  int read() { return 0; }
};

extern TwoWire Wire;
