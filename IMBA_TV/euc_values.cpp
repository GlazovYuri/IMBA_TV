#include "euc_values.h"
#include "config.h"
#include <Arduino.h>

// номера в config.json должны совпадать с EUC_VALUE_* и SCREEN_* (у всех экранов одинаковые варианты,
// это проверяют тесты сайта)
static constexpr bool same(int a, int b) { return a == b; }
static_assert(same(CONFIG_SCREEN1_V1_SPEED, EUC_VALUE_SPEED) && same(CONFIG_SCREEN1_V1_PWM, EUC_VALUE_PWM) &&
              same(CONFIG_SCREEN1_V1_CHARGE, EUC_VALUE_CHARGE) && same(CONFIG_SCREEN1_V1_VOLTAGE, EUC_VALUE_VOLTAGE) &&
              same(CONFIG_SCREEN1_V1_TEMPERATURE, EUC_VALUE_TEMPERATURE) && same(CONFIG_SCREEN1_V1_TRIP, EUC_VALUE_TRIP),
              "config.json: номера значений разошлись с EUC_VALUE_*");
static_assert(same(CONFIG_SCREEN1_TYPE_NONE, SCREEN_NONE) && same(CONFIG_SCREEN1_TYPE_MAIN, SCREEN_MAIN) &&
              same(CONFIG_SCREEN1_TYPE_GRID, SCREEN_GRID) && same(CONFIG_SCREEN1_TYPE_INFO, SCREEN_INFO) &&
              same(CONFIG_SCREEN1_TYPE_CLOCK, SCREEN_CLOCK),
              "config.json: номера типов экранов разошлись с SCREEN_*");

void eucValueText(uint8_t value, const euc_data_t& data, char* text, size_t size)
{
  switch(value)
  {
    case EUC_VALUE_SPEED:
      if(data.present & EUC_HAS_SPEED) snprintf(text, size, "%u", (unsigned)constrain(data.speed, 0, 999));
      else snprintf(text, size, "--");
      break;

    case EUC_VALUE_PWM:
      if(data.present & EUC_HAS_PWM) snprintf(text, size, "%u", (unsigned)data.pwm);
      else snprintf(text, size, "--");
      break;

    case EUC_VALUE_CHARGE:
      if(data.present & EUC_HAS_CHARGE) snprintf(text, size, "%u", (unsigned)constrain(data.charge, 0, 100));
      else snprintf(text, size, "--");
      break;

    case EUC_VALUE_VOLTAGE:
      if(data.present & EUC_HAS_VOLTAGE) snprintf(text, size, "%u.%u", data.voltage_x10 / 10, data.voltage_x10 % 10);
      else snprintf(text, size, "--.-");
      break;

    // целые градусы
    case EUC_VALUE_TEMPERATURE:
      if(data.present & EUC_HAS_TEMPERATURE)
      {
        int t = data.temperature_x10;
        t = (t >= 0) ? (t + 5) / 10 : -((-t + 5) / 10);
        snprintf(text, size, "%d", constrain(t, -99, 999));
      }
      else snprintf(text, size, "--");
      break;

    // от 1000 км без десятых
    case EUC_VALUE_TRIP:
      if(!(data.present & EUC_HAS_TRIP)) snprintf(text, size, "--.-");
      else if(data.trip_x10 >= 10000) snprintf(text, size, "%u", data.trip_x10 / 10);
      else snprintf(text, size, "%u.%u", data.trip_x10 / 10, data.trip_x10 % 10);
      break;

    default:
      snprintf(text, size, "--");
      break;
  }
}
