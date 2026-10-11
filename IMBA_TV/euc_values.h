#pragma once
#include <stddef.h>
#include <stdint.h>
#include "euc_data.h"

// Значения колеса, которые выбираются в настройках для основного экрана и сетки.
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

// значение текстом: "27", "141.8"; прочерки, если колесо его не прислало
void eucValueText(uint8_t value, const euc_data_t& data, char* text, size_t size);
