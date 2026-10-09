#pragma once
#include <stdint.h>

struct SPISettings
{
  SPISettings(uint32_t, uint8_t, uint8_t) {}
  SPISettings() {}
};

class SPIClass
{
public:
  void begin() {}
  void beginTransaction(SPISettings) {}
  void endTransaction() {}
  uint8_t transfer(uint8_t) { return 0; }
};

extern SPIClass SPI;
#define MSBFIRST 1
#define SPI_MODE0 0
