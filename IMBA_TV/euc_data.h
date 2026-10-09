#pragma once
#include <stdint.h>

// биты present: какие поля пришли в последнем пакете (порядок как в протоколе LoEUC)
enum
{
  EUC_HAS_SPEED = 1 << 0,
  EUC_HAS_PWM = 1 << 1,
  EUC_HAS_VOLTAGE = 1 << 2,
  EUC_HAS_CHARGE = 1 << 3,
  EUC_HAS_TEMPERATURE = 1 << 4,
  EUC_HAS_TRIP = 1 << 5,
  EUC_HAS_PWM_LIMIT = 1 << 6,
};

struct euc_data_t
{
  bool is_connected;
  uint16_t speed;
  uint8_t pwm;
  uint8_t charge;
  uint8_t pwm_limit;
  uint8_t frame_cnt;

  uint8_t present;
  uint16_t voltage_x10;     // В * 10
  int16_t temperature_x10;  // °C * 10
  uint16_t trip_x10;        // км * 10

  uint16_t frame_interval_ms;  // среднее время между кадрами, 0 = ещё не измерено
  uint32_t last_frame_ms;      // millis() последнего кадра

  euc_data_t()
    : is_connected(false), speed(0), pwm(0), charge(0), pwm_limit(UINT8_MAX), frame_cnt(0),
      present(0), voltage_x10(0), temperature_x10(0), trip_x10(0),
      frame_interval_ms(0), last_frame_ms(0) {}

  void reset()
  {
    is_connected = false;
    speed = 0;
    pwm = 0;
    charge = 0;
    pwm_limit = UINT8_MAX;
    present = 0;
    voltage_x10 = 0;
    temperature_x10 = 0;
    trip_x10 = 0;
    frame_interval_ms = 0;
    last_frame_ms = 0;
  }
};
