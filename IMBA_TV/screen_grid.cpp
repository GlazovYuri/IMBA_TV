#include "screen_grid.h"
#include "screen_text.h"
#include "euc_values.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

static const int scr_width = 128;
// промежуток между числом и единицей и наименьший просвет между столбцами, px
static const int unit_gap = 3;
static const int column_gap = 6;

// ширина единицы измерения вместе с промежутком после последнего знака (знак 6 px)
static int unitWidth(uint8_t value)
{
  switch(value)
  {
    case EUC_VALUE_SPEED: return 24;        // км/ч
    case EUC_VALUE_PWM: return 18;          // ШИМ
    case EUC_VALUE_TEMPERATURE: return 10;  // знак градуса и C
    case EUC_VALUE_TRIP: return 12;         // км
    default: return 6;                      // % и В
  }
}

static void drawUnit(int x, int y, uint8_t value)
{
  switch(value)
  {
    case EUC_VALUE_SPEED: screenTextUnit(x, y, "км/ч"); break;
    case EUC_VALUE_PWM: screenTextUnit(x, y, "ШИМ"); break;
    case EUC_VALUE_CHARGE: screenTextUnit(x, y, "%"); break;
    case EUC_VALUE_VOLTAGE: screenTextUnit(x, y, "В"); break;
    case EUC_VALUE_TEMPERATURE:
      oled.rect(x, y + 7, x + 2, y + 9, OLED_STROKE);  // знак градуса
      screenTextUnit(x + 4, y, "C");
      break;
    case EUC_VALUE_TRIP: screenTextUnit(x, y, "км"); break;
  }
}

// ширина самого длинного числа значения: под него оставляется место, чтобы числа не прыгали
static int numberWidth(uint8_t value, const euc_data_t& data)
{
  char text[12];
  if(value == EUC_VALUE_VOLTAGE)
  {
    uint16_t voltage_max_x10 = eucVoltageMax(data);
    snprintf(text, sizeof(text), "%u.%u", voltage_max_x10 / 10, voltage_max_x10 % 10);
  }
  else if(value == EUC_VALUE_TRIP) snprintf(text, sizeof(text), "999.9");
  else snprintf(text, sizeof(text), "100");  // скорость, ШИМ, заряд, температура: до трёх знаков
  return screenTextBigWidth(text);
}

// место клетки: правый край числа и начало единицы.
// fit_text: число прижато к левому краю экрана, единица сразу за ним - место по самому длинному числу не помещается
struct cell_t
{
  int right;
  int unit_x;
  bool fit_text;
};

// левый столбец от левого края экрана, отступ 3 px
static cell_t placeLeft(int number_width)
{
  cell_t c;
  c.right = number_width + 2;
  c.unit_x = c.right + unit_gap;
  c.fit_text = false;
  return c;
}

// правый столбец: единицы у правого края экрана
static cell_t placeRight(int unit_width)
{
  cell_t c;
  c.unit_x = scr_width - 1 - unit_width;
  c.right = c.unit_x - unit_gap;
  c.fit_text = false;
  return c;
}

// между левой клеткой и самым длинным числом правой остаётся просвет column_gap
static bool fits(const cell_t& left, int left_unit_width, const cell_t& right, int right_number_width)
{
  int left_last = left.unit_x + left_unit_width - 2;
  int right_first = right.right - right_number_width + 1;
  return right_first - left_last - 1 >= column_gap;
}

void screenGridDraw(euc_data_t& data, const uint8_t values[4])
{
  const int row_y[2] = {7, 37};

  int number_w[4], unit_w[4];
  for(int i = 0; i < 4; i++)
  {
    number_w[i] = numberWidth(values[i], data);
    unit_w[i] = unitWidth(values[i]);
  }

  // в столбце числа выровнены по правому краю, единицы начинаются с одного места.
  // Если так столбцы не помещаются рядом, каждая строка размечается сама по себе,
  // а если не помещается и строка, левое число прижимается к краю экрана
  cell_t cells[4];
  cells[0] = cells[2] = placeLeft(max(number_w[0], number_w[2]));
  cells[1] = cells[3] = placeRight(max(unit_w[1], unit_w[3]));
  if(!fits(cells[0], max(unit_w[0], unit_w[2]), cells[1], max(number_w[1], number_w[3])))
  {
    for(int i = 0; i < 4; i += 2)
    {
      cells[i] = placeLeft(number_w[i]);
      cells[i + 1] = placeRight(unit_w[i + 1]);
      cells[i].fit_text = !fits(cells[i], unit_w[i], cells[i + 1], number_w[i + 1]);
    }
  }

  char text[12];
  for(int i = 0; i < 4; i++)
  {
    int y = row_y[i / 2];
    eucValueText(values[i], data, text, sizeof(text));
    // в тесной строке пробег от 100 км без десятых, иначе он упирается в соседнее значение
    bool crowded = cells[i & ~1].fit_text;
    if(crowded && values[i] == EUC_VALUE_TRIP && (data.present & EUC_HAS_TRIP) && data.trip_x10 >= 1000)
      snprintf(text, sizeof(text), "%u", data.trip_x10 / 10);
    int width = screenTextBigWidth(text);
    cell_t c = cells[i].fit_text ? placeLeft(width) : cells[i];
    // число длиннее отведённого места (напряжение выше выбранной батареи) прижимается к левому краю экрана
    if(c.right - width + 1 < 3) c = placeLeft(width);
    screenTextBig(c.right, y, text);
    drawUnit(c.unit_x, y, values[i]);
  }
}
