#include "euc_values.h"
#include "config.h"
#include <Arduino.h>

// номера в config.json должны совпадать с EUC_VALUE_* и SCREEN_* (у всех экранов одинаковые варианты,
// это проверяют тесты сайта)
static constexpr bool same(int a, int b) { return a == b; }
static_assert(same(CONFIG_SCREEN1_V1_SPEED, EUC_VALUE_SPEED) && same(CONFIG_SCREEN1_V1_PWM, EUC_VALUE_PWM) &&
              same(CONFIG_SCREEN1_V1_CHARGE, EUC_VALUE_CHARGE) && same(CONFIG_SCREEN1_V1_VOLTAGE, EUC_VALUE_VOLTAGE) &&
              same(CONFIG_SCREEN1_V1_TEMPERATURE, EUC_VALUE_TEMPERATURE) && same(CONFIG_SCREEN1_V1_TRIP, EUC_VALUE_TRIP) &&
              same(CONFIG_SCREEN1_V1_CLOCK, EUC_VALUE_CLOCK),
              "config.json: номера значений разошлись с EUC_VALUE_*");
static_assert(same(CONFIG_SCREEN1_TYPE_NONE, SCREEN_NONE) && same(CONFIG_SCREEN1_TYPE_MAIN, SCREEN_MAIN) &&
              same(CONFIG_SCREEN1_TYPE_GRID, SCREEN_GRID) && same(CONFIG_SCREEN1_TYPE_INFO, SCREEN_INFO) &&
              same(CONFIG_SCREEN1_TYPE_CLOCK, SCREEN_CLOCK),
              "config.json: номера типов экранов разошлись с SCREEN_*");

// батареи из настроек: ячеек последовательно
static const uint8_t battery_series[] = {20, 24, 30, 32, 36, 40, 42};
// напряжение ячейки при заряде 0 и 100 %, между ними в первом приближении линейно
static const uint32_t cell_empty_mv = 3300;
static const uint32_t cell_full_mv = 4200;
// ячейка с напряжением вне этих пределов значит, что батарея не подошла ни одна
static const uint32_t cell_min_mv = 3000;
static const uint32_t cell_max_mv = 4350;
// столько кадров подряд с одним ответом, около 2 с
static const uint8_t detect_frames = 10;
// быстрее колесо едет под нагрузкой, и напряжение проседает
static const uint16_t detect_speed_max = 3; //km/h
static const uint16_t voltage_max_default_x10 = 1512;

uint16_t eucVoltageMax(const euc_data_t& data)
{
  uint16_t cfg = config().wheel_voltage_x10;
  if(cfg != CONFIG_WHEEL_VOLTAGE_X10_AUTO) return cfg;
  return data.battery_max_x10 ? data.battery_max_x10 : voltage_max_default_x10;
}

void eucBatteryDetect(euc_data_t& data)
{
  static uint8_t candidate = 0, count = 0;
  bool usable = (data.present & EUC_HAS_VOLTAGE) && (data.present & EUC_HAS_CHARGE) &&
                !((data.present & EUC_HAS_SPEED) && data.speed > detect_speed_max);
  if(!usable)
  {
    count = 0;
    return;
  }

  // ближайшая батарея по отношению напряжений: у больших батарей шаг между соседними меньше
  uint32_t mv = (uint32_t)data.voltage_x10 * 100;
  uint32_t cell_mv = cell_empty_mv + (cell_full_mv - cell_empty_mv) * data.charge / 100;
  uint8_t best = 0;
  uint32_t best_err = UINT32_MAX;
  for(uint8_t s : battery_series)
  {
    uint32_t expect = s * cell_mv;
    uint32_t err = (uint32_t)((mv > expect ? mv - expect : expect - mv) * 1000ULL / expect);
    if(err < best_err)
    {
      best_err = err;
      best = s;
    }
  }
  uint32_t per_cell = mv / best;
  if(per_cell < cell_min_mv || per_cell > cell_max_mv)
  {
    count = 0;
    return;
  }

  if(best != candidate)
  {
    candidate = best;
    count = 0;
  }
  if(count < detect_frames) count++;
  if(count >= detect_frames) data.battery_max_x10 = best * cell_full_mv / 100;
}

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
