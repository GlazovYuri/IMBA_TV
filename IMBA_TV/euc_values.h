#pragma once
#include <stddef.h>
#include <stdint.h>
#include "euc_data.h"

// Значения колеса, которые выбираются в настройках экранов (поля screenN_v1..v4).
// Номера совпадают с options в config.json
enum
{
  EUC_VALUE_SPEED = 0,
  EUC_VALUE_PWM = 1,
  EUC_VALUE_CHARGE = 2,
  EUC_VALUE_VOLTAGE = 3,
  EUC_VALUE_TEMPERATURE = 4,
  EUC_VALUE_TRIP = 5,
};

// Типы экранов (screenN_type)
enum
{
  SCREEN_NONE = 0,  // экрана нет
  SCREEN_MAIN = 1,  // два крупных значения, сверху заряд или напряжение колеса (screen_main)
  SCREEN_GRID = 2,  // сетка из четырёх значений (screen_grid)
  SCREEN_INFO = 3,  // связь и версии (screen_info)
};

// значение текстом: "27", "141.8"; прочерки, если колесо его не прислало
void eucValueText(uint8_t value, const euc_data_t& data, char* text, size_t size);
