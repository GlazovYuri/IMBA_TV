#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
class Print {
 public:
  virtual size_t write(uint8_t) = 0;
  size_t print(const char* s) { size_t n = 0; while (*s) n += write((uint8_t)*s++); return n; }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(long v) { char b[24]; snprintf(b, sizeof b, "%ld", v); return print(b); }
  size_t print(int v) { return print((long)v); }
  size_t print(unsigned int v) { return print((long)v); }
  size_t print(unsigned char v, int = 10) { return print((long)v); }
  size_t print(unsigned short v) { return print((long)v); }
  size_t print(unsigned long v) { return print((long)v); }
};
