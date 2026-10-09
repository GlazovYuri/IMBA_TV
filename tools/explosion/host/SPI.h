#pragma once
#include <stdint.h>
#define MSBFIRST 1
#define SPI_MODE0 0
struct SPISettings { SPISettings(uint32_t, int, int) {} };
struct SPIClass { void begin() {} void beginTransaction(SPISettings) {} void endTransaction() {} void transfer(uint8_t) {} };
static SPIClass SPI;
