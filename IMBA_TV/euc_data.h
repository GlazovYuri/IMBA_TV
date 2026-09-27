#pragma once
#include <stdint.h>

struct euc_data_t
{
  bool is_connected;
  uint16_t speed;
  uint8_t pwm;
  uint8_t charge;
  uint8_t pwm_limit;
  uint8_t frame_cnt;

  euc_data_t()
    : is_connected(false), speed(0), pwm(0), charge(0), pwm_limit(UINT8_MAX), frame_cnt(0) {}

  void reset()
  {
    is_connected = false;
    speed = 0;
    pwm = 0;
    charge = 0;
    pwm_limit = UINT8_MAX;
  }
};
