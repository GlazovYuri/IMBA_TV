#include "screen_main.h"
#include "config.h"
#include "euc_values.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

static const int scr_width = 128;

static const uint8_t charging_bitmap[] = {
  0xff, 0x81, 0xb1, 0x99, 0x8d, 0x9d, 0xb9, 0xb1, 0x99, 0x8d, 0x81, 0xff, 0x3c, 0x3c
};

static void drawDevCharge(int x, int y, uint8_t charge, bool is_charging)
{
  charge = constrain(charge, 0, 100);

  if(is_charging)
  {
    oled.drawBitmap(x, y, charging_bitmap, 14, 8);
  }
  else
  {
    oled.setCursorXY(x, y);

    oled.drawByte(0xFF);

    uint8_t level = charge / 10;
    for(uint8_t i = 0; i < 10; i++)
    {
      if(i < level) oled.drawByte(0xFF);
      else oled.drawByte(0x81);
    }

    oled.drawByte(0xFF);
    oled.drawByte(0x3C);
    oled.drawByte(0x3C);
  }

  oled.setCursorXY(x + 17, y);
  oled.setScale(1);
  oled.print(charge);
  oled.print("%");
}

// полоска заряда колеса: рамка + cells делений по 2 px, ширина 2 * cells + 4
static void drawEucBar(int x, int y, uint8_t charge, uint8_t cells)
{
  charge = constrain(charge, 0, 100);

  oled.setCursorXY(x, y);

  oled.drawByte(0xFF);

  uint8_t level = charge * cells / 100;
  for(uint8_t i = 0; i < cells; i++)
  {
    if(i < level)
    {
      oled.drawByte(0x81);
      oled.drawByte(0xFF);
    }
    else
    {
      oled.drawByte(0x81);
      oled.drawByte(0x81);
    }
  }

  oled.drawByte(0xFF);
  oled.drawByte(0x3C);
  oled.drawByte(0x3C);
}

static void drawEucCharge(int x, int y, uint8_t charge)
{
  charge = constrain(charge, 0, 100);
  drawEucBar(x, y, charge, 25);

  oled.setCursorXY(x + 57, y);
  oled.setScale(1);
  oled.print(charge);
  oled.print("%");
}

// напряжение колеса вместо процентов: текст прижат к правому краю,
// место под него рассчитано по максимальному напряжению, полоска укорачивается
static void drawEucVoltage(int x, int y, euc_data_t& data)
{
  char text[12];
  if(data.present & EUC_HAS_VOLTAGE)
    snprintf(text, sizeof(text), "%u.%u", data.voltage_x10 / 10, data.voltage_x10 % 10);
  else
    snprintf(text, sizeof(text), "--.-");

  // по максимальному напряжению колеса резервируется место под текст
  uint16_t voltage_max_x10 = config().wheel_voltage_x10;
  char max_text[12];
  snprintf(max_text, sizeof(max_text), "%u.%u", voltage_max_x10 / 10, voltage_max_x10 % 10);

  // + 1 знакоместо под "В"
  int field_x = scr_width - (strlen(max_text) + 1) * 6;
  int cells = (field_x - 3 - x - 4) / 2;
  drawEucBar(x, y, data.charge, cells);

  oled.setCursorXY(scr_width - (strlen(text) + 1) * 6, y);
  oled.setScale(1);
  oled.print(text);
  oled.print("В");
}

// подпись под крупным значением
static const char* bigLabel(uint8_t value)
{
  switch(value)
  {
    case EUC_VALUE_SPEED: return "скорость";
    case EUC_VALUE_PWM: return "ШИМ";
    case EUC_VALUE_TEMPERATURE: return "температура";
    default: return "";
  }
}

// число знаков: в UTF-8 считаем только первые байты символов
static int textChars(const char* text)
{
  int chars = 0;
  for(const char* p = text; *p; p++) if((*p & 0xC0) != 0x80) chars++;
  return chars;
}

// левый край текста шириной width по центру места [left, right], но не за его пределами.
// center2 - удвоенная середина: у места чётной ширины она между столбцами
static int centerX(int center2, int width, int left, int right)
{
  int x = (center2 - width + 2) / 2;
  return constrain(x, left, right - width + 1);
}

// крупное значение и подпись под ним, на своём месте: слева x 0..71, справа 72..127.
// Число, которое не помещается шрифтом scale 4, выводится мельче, по нижнему краю крупного
static void drawBigValue(int left, int right, int center2, int y, uint8_t value, euc_data_t& data)
{
  char text[12];
  eucValueText(value, data, text, sizeof(text));
  int chars = textChars(text);

  uint8_t scale = 4;
  while(scale > 2 && chars * 6 * scale - scale > right - left + 1) scale--;
  int width = chars * 6 * scale - scale;
  // мелкое число - по середине всего места, так дальше от соседнего
  int number_center2 = (scale == 4) ? center2 : left + right;
  oled.setCursorXY(centerX(number_center2, width, left, right), y + 7 * (4 - scale));
  oled.setScale(scale);
  oled.print(text);

  // подпись может выйти за своё место: «температура» справа шире его
  const char* label = bigLabel(value);
  width = textChars(label) * 6 - 1;
  oled.setCursorXY(centerX(center2, width, 0, scr_width - 1), y + 38);
  oled.setScale(1);
  oled.print(label);
}

void screenMainDraw(uint8_t dev_charge, bool is_charging, euc_data_t& data, uint8_t top, uint8_t left, uint8_t right)
{
  drawDevCharge(0, 1, dev_charge, is_charging);
  if(top == EUC_VALUE_VOLTAGE) drawEucVoltage(47, 1, data);
  else drawEucCharge(47, 1, data.charge);

  // середины мест - как у трёх знаков слева и двух справа: x 0..67 и 72..115
  drawBigValue(0, 71, 67, 18, left, data);
  drawBigValue(72, scr_width - 1, 187, 18, right, data);
}

void screenNoLinkDraw(uint8_t dev_charge, bool is_charging)
{
  drawDevCharge(0, 1, dev_charge, is_charging);
  oled.setCursor(10, 3);
  oled.setScale(2);
  oled.print("нет связи");
}
