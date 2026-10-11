#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

class Print
{
public:
  virtual ~Print() {}
  virtual size_t write(uint8_t c) = 0;
  virtual size_t write(const uint8_t* buf, size_t n)
  {
    size_t r = 0;
    while(n--) r += write(*buf++);
    return r;
  }
  size_t write(const char* s) { return s ? write((const uint8_t*)s, strlen(s)) : 0; }
  size_t write(const char* s, size_t n) { return write((const uint8_t*)s, n); }

  size_t print(const char* s) { return write(s); }
  size_t print(char c) { return write((uint8_t)c); }
  size_t print(unsigned char v, int base = 10) { return printNum((unsigned long)v, base); }
  size_t print(int v, int base = 10) { return printSigned(v, base); }
  size_t print(unsigned int v, int base = 10) { return printNum(v, base); }
  size_t print(long v, int base = 10) { return printSigned(v, base); }
  size_t print(unsigned long v, int base = 10) { return printNum(v, base); }
  size_t print(double v, int digits = 2);

  size_t println() { return write("\r\n"); }
  template <class T>
  size_t println(T v) { size_t n = print(v); return n + println(); }
  template <class T>
  size_t println(T v, int arg) { size_t n = print(v, arg); return n + println(); }

  size_t printf(const char* fmt, ...) __attribute__((format(printf, 2, 3)));

private:
  size_t printNum(unsigned long v, int base);
  size_t printSigned(long v, int base);
};
