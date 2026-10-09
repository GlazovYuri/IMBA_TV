#include "screen_grid.h"
#include "screen_text.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

void screenGridDraw(euc_data_t& data)
{
  // левый столбец: до 3 знаков, правый: до 5 (с точкой)
  const int left_right = 36;
  const int left_unit = 39;
  const int right_right = 112;
  const int right_unit = 115;
  const int row_y[2] = {7, 37};
  char text[12];

  // заряд
  if(data.present & EUC_HAS_CHARGE) snprintf(text, sizeof(text), "%u", (unsigned)constrain(data.charge, 0, 100));
  else snprintf(text, sizeof(text), "--");
  screenTextBig(left_right, row_y[0], text);
  screenTextUnit(left_unit, row_y[0], "%");

  // напряжение
  if(data.present & EUC_HAS_VOLTAGE) snprintf(text, sizeof(text), "%u.%u", data.voltage_x10 / 10, data.voltage_x10 % 10);
  else snprintf(text, sizeof(text), "--.-");
  screenTextBig(right_right, row_y[0], text);
  screenTextUnit(right_unit, row_y[0], "В");

  // температура, целые градусы
  if(data.present & EUC_HAS_TEMPERATURE)
  {
    int t = data.temperature_x10;
    t = (t >= 0) ? (t + 5) / 10 : -((-t + 5) / 10);
    snprintf(text, sizeof(text), "%d", constrain(t, -99, 999));
  }
  else snprintf(text, sizeof(text), "--");
  screenTextBig(left_right, row_y[1], text);
  oled.rect(left_unit, row_y[1] + 7, left_unit + 2, row_y[1] + 9, OLED_STROKE);  // знак градуса
  screenTextUnit(left_unit + 4, row_y[1], "C");

  // пробег: от 1000 км без десятых
  if(data.present & EUC_HAS_TRIP)
  {
    if(data.trip_x10 >= 10000) snprintf(text, sizeof(text), "%u", data.trip_x10 / 10);
    else snprintf(text, sizeof(text), "%u.%u", data.trip_x10 / 10, data.trip_x10 % 10);
  }
  else snprintf(text, sizeof(text), "--.-");
  screenTextBig(right_right, row_y[1], text);
  screenTextUnit(right_unit, row_y[1], "км");
}
