#pragma once
#include "Print.h"

// Serial пишет в журнал эмулятора
class HardwareSerial : public Print
{
public:
  void begin(unsigned long) {}
  void end() {}
  operator bool() const { return true; }
  int available() { return 0; }
  int read() { return -1; }
  void flush();
  size_t write(uint8_t c) override;
  using Print::write;
};

extern HardwareSerial Serial;
extern HardwareSerial Serial1;
