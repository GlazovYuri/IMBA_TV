#pragma once
#include <stddef.h>
#include <stdint.h>

// I2C (TWIM). На шине висит модель контроллера SH1106 по адресу 0x3C.
class TwoWire
{
public:
  void begin();
  void end();
  void setClock(uint32_t hz);
  void beginTransmission(uint8_t addr);
  void beginTransmission(int addr) { beginTransmission((uint8_t)addr); }
  size_t write(uint8_t b);
  size_t write(const uint8_t* buf, size_t n);
  uint8_t endTransmission(bool stop = true);
  uint8_t requestFrom(int addr, int qty, bool stop = true);
  int available();
  int read();

private:
  static constexpr size_t kBuf = 64; // SERIAL_BUFFER_SIZE в ядре nRF52
  uint8_t addr_ = 0;
  uint8_t tx_[kBuf];
  size_t tx_len_ = 0;
  bool tx_overflow_ = false;
  uint8_t rx_[kBuf];
  size_t rx_len_ = 0;
  size_t rx_pos_ = 0;
};

extern TwoWire Wire;
