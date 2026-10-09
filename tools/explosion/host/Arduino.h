// Заглушка Arduino для сборки display.cpp на компьютере (только для рендера превью).
#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <algorithm>
typedef uint8_t byte;
#define PROGMEM
#define pgm_read_byte(a) (*(const uint8_t*)(a))
#define pgm_read_word(a) (*(const uint16_t*)(a))
#define bitWrite(v, b, bit) ((bit) ? ((v) |= (1UL << (b))) : ((v) &= ~(1UL << (b))))
#define bitRead(v, b) (((v) >> (b)) & 1)
#define constrain(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))
#define INPUT 0
#define OUTPUT 1
#define PIN_WIRE_SCL 0
#define PIN_WIRE_SDA 1
inline long map(long x, long a, long b, long c, long d) { return (x - a) * (d - c) / (b - a) + c; }
inline uint32_t millis() { return 0; }
inline uint32_t micros() { return 0; }
inline void (*g_delay_hook)(uint32_t) = nullptr;
inline void delay(uint32_t ms) { if (g_delay_hook) g_delay_hook(ms); }
inline void delayMicroseconds(uint32_t) {}
inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
#include "Print.h"
