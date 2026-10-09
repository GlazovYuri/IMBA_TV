#include "screen_grid.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

static const int scr_width = 128;
// кадров от телефона нет дольше этого времени - связь показывается прочерками
static const uint32_t link_stall_ms = 3000;

// крупный текст (scale 2) с узкой десятичной точкой, прижат к правому краю right (последний видимый столбец)
static void drawBigText(int right, int y, const char* text)
{
  // знак: 10 px + 2 px промежуток, точка: 2 px + 2 px промежуток
  int width = 0;
  for(const char* p = text; *p; p++) width += (*p == '.') ? 4 : 12;

  int x = right - (width - 2) + 1;
  oled.setScale(2);
  for(const char* p = text; *p; p++)
  {
    if(*p == '.')
    {
      oled.rect(x, y + 12, x + 1, y + 13, OLED_FILL);
      x += 4;
      continue;
    }
    oled.setCursorXY(x, y);
    oled.print(*p);
    x += 12;
  }
}

static void drawUnit(int x, int y, const char* unit)
{
  oled.setScale(1);
  oled.setCursorXY(x, y + 7);
  oled.print(unit);
}

void screenGridDraw(euc_data_t& data)
{
  // левый столбец: до 3 знаков, правый: до 5 (с точкой)
  const int left_right = 36;
  const int left_unit = 39;
  const int right_right = 112;
  const int right_unit = 115;
  const int row_y[3] = {2, 21, 44};
  const int separator_y = 39;
  char text[12];

  // заряд
  if(data.present & EUC_HAS_CHARGE) snprintf(text, sizeof(text), "%u", (unsigned)constrain(data.charge, 0, 100));
  else snprintf(text, sizeof(text), "--");
  drawBigText(left_right, row_y[0], text);
  drawUnit(left_unit, row_y[0], "%");

  // напряжение
  if(data.present & EUC_HAS_VOLTAGE) snprintf(text, sizeof(text), "%u.%u", data.voltage_x10 / 10, data.voltage_x10 % 10);
  else snprintf(text, sizeof(text), "--.-");
  drawBigText(right_right, row_y[0], text);
  drawUnit(right_unit, row_y[0], "В");

  // температура, целые градусы
  if(data.present & EUC_HAS_TEMPERATURE)
  {
    int t = data.temperature_x10;
    t = (t >= 0) ? (t + 5) / 10 : -((-t + 5) / 10);
    snprintf(text, sizeof(text), "%d", constrain(t, -99, 999));
  }
  else snprintf(text, sizeof(text), "--");
  drawBigText(left_right, row_y[1], text);
  oled.rect(left_unit, row_y[1] + 7, left_unit + 2, row_y[1] + 9, OLED_STROKE);  // знак градуса
  drawUnit(left_unit + 4, row_y[1], "C");

  // пробег: от 1000 км без десятых
  if(data.present & EUC_HAS_TRIP)
  {
    if(data.trip_x10 >= 10000) snprintf(text, sizeof(text), "%u", data.trip_x10 / 10);
    else snprintf(text, sizeof(text), "%u.%u", data.trip_x10 / 10, data.trip_x10 % 10);
  }
  else snprintf(text, sizeof(text), "--.-");
  drawBigText(right_right, row_y[1], text);
  drawUnit(right_unit, row_y[1], "км");

  // связь: среднее время между кадрами и частота, "--" если кадров нет дольше link_stall_ms
  bool link_ok = data.frame_interval_ms != 0 && (millis() - data.last_frame_ms) < link_stall_ms;

  // до секунды - в мс, дальше - в секундах с десятыми
  const char* interval_unit = "мс";
  if(!link_ok) snprintf(text, sizeof(text), "--");
  else if(data.frame_interval_ms < 1000) snprintf(text, sizeof(text), "%u", (unsigned)data.frame_interval_ms);
  else
  {
    unsigned s_x10 = (data.frame_interval_ms + 50) / 100;
    if(s_x10 > 999) s_x10 = 999;
    snprintf(text, sizeof(text), "%u.%u", s_x10 / 10, s_x10 % 10);
    interval_unit = "с";
  }
  drawBigText(left_right, row_y[2], text);
  drawUnit(left_unit, row_y[2], interval_unit);

  if(link_ok)
  {
    uint32_t hz_x10 = (10000UL + data.frame_interval_ms / 2) / data.frame_interval_ms;
    snprintf(text, sizeof(text), "%lu.%lu", (unsigned long)(hz_x10 / 10), (unsigned long)(hz_x10 % 10));
  }
  else snprintf(text, sizeof(text), "--.-");
  drawBigText(right_right, row_y[2], text);
  drawUnit(right_unit, row_y[2], "Гц");

  // черта последней: крупный текст пишет в экран целыми байтами и стёр бы её
  oled.fastLineH(separator_y, 0, scr_width - 1);
}
