#pragma once
#include <stdint.h>

// SPI на плате не используется; нужно только чтобы собрался GyverOLED
#define SPI_MODE0 0x00
#define SPI_MODE1 0x01
#define SPI_MODE2 0x02
#define SPI_MODE3 0x03

class SPISettings
{
public:
  SPISettings(uint32_t, uint8_t, uint8_t) {}
  SPISettings() {}
};

class SPIClass
{
public:
  void begin() {}
  void end() {}
  void beginTransaction(SPISettings) {}
  void endTransaction() {}
  uint8_t transfer(uint8_t) { return 0; }
};

extern SPIClass SPI;
