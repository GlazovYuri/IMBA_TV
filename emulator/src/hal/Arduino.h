#pragma once
// Arduino API для сборки прошивки на компьютере (подмножество ядра Adafruit nRF52).
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "variant.h"

typedef uint8_t byte;
typedef bool boolean;

#define HIGH 0x1
#define LOW 0x0

#define INPUT (0x0)
#define OUTPUT (0x1)
#define INPUT_PULLUP (0x2)
#define INPUT_PULLDOWN (0x3)
#define INPUT_PULLUP_SENSE (0x4)
#define INPUT_PULLDOWN_SENSE (0x5)

#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2

#define LSBFIRST 0
#define MSBFIRST 1

#define PROGMEM
#define F(s) (s)

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))
#define sq(x) ((x) * (x))
#define radians(deg) ((deg) * (PI / 180.0))
#define degrees(rad) ((rad) * (180.0 / PI))
#define bit(b) (1UL << (b))
#define bitRead(value, b) (((value) >> (b)) & 0x01)
#define bitSet(value, b) ((value) |= (1UL << (b)))
#define bitClear(value, b) ((value) &= ~(1UL << (b)))
#define bitWrite(value, b, bitvalue) ((bitvalue) ? bitSet(value, b) : bitClear(value, b))
#define lowByte(w) ((uint8_t)((w) & 0xff))
#define highByte(w) ((uint8_t)((w) >> 8))

// как в ядре Adafruit: шаблоны вместо макросов
template <class T, class L>
static inline auto min(const T& a, const L& b) -> decltype((b < a) ? b : a)
{
  return (b < a) ? b : a;
}

template <class T, class L>
static inline auto max(const T& a, const L& b) -> decltype((b < a) ? b : a)
{
  return (a < b) ? b : a;
}

// GyverOLED читает pgm_read_word из байтового массива (последний байт +1 за границей),
// на железе это безвредно - не даём санитайзеру ругаться на библиотеку
__attribute__((no_sanitize("address"))) static inline uint16_t pgm_read_word(const void* p)
{
  const uint8_t* b = (const uint8_t*)p;
  return (uint16_t)(b[0] | (b[1] << 8));
}
#define pgm_read_byte(p) (*(const uint8_t*)(p))
#define pgm_read_dword(p) (*(const uint32_t*)(p))

enum eAnalogReference
{
  AR_DEFAULT,
  AR_INTERNAL,
  AR_INTERNAL_3_0,
  AR_INTERNAL_2_4,
  AR_INTERNAL_1_8,
  AR_INTERNAL_1_2,
  AR_VDD4,
};

uint32_t millis();
uint32_t micros();
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
void yield();

void pinMode(uint32_t pin, uint32_t mode);
void digitalWrite(uint32_t pin, uint32_t val);
int digitalRead(uint32_t pin);
void digitalToggle(uint32_t pin);

void analogReference(eAnalogReference ref);
void analogReadResolution(int bits);
void analogSampleTime(uint8_t us);
uint32_t analogRead(uint32_t pin);
uint32_t analogReadVDD();
uint32_t analogReadVDDHDIV5();

long map(long x, long in_min, long in_max, long out_min, long out_max);
long random(long max);
long random(long min, long max);
void randomSeed(unsigned long seed);

uint32_t readResetReason();
void initVariant();

#include "Print.h"
#include "HardwareSerial.h"
