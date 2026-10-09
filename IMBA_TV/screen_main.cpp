#include "screen_main.h"
#include "config.h"
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

static int cntDigits(uint16_t val)
{
  if(val == 0) return 1;

  int cnt = 0;
  while(val != 0)
  {
    val /= 10;
    ++cnt;
  }
  return cnt;
}

static void drawEucSpeed(int x, int y, uint16_t speed)
{
  speed = constrain(speed, 0, 999);

  int digits = cntDigits(speed);
  oled.setCursorXY(x + (3 - digits) * 12, y);
  oled.setScale(4);
  oled.print(speed);

  oled.setCursorXY(x + 11, y + 38);
  oled.setScale(1);
  oled.print("скорость");
}

static void drawEucPwm(int x, int y, uint8_t pwm)
{
  pwm = pwm % 100;

  int digits = cntDigits(pwm);
  oled.setCursorXY(x + (2 - digits) * 12, y);
  oled.setScale(4);
  oled.print(pwm);

  oled.setCursorXY(x + 14, y + 38);
  oled.setScale(1);
  oled.print("ШИМ");
}

void screenMainDraw(uint8_t dev_charge, bool is_charging, euc_data_t& data, bool show_voltage)
{
  drawDevCharge(0, 1, dev_charge, is_charging);
  if(show_voltage) drawEucVoltage(47, 1, data);
  else drawEucCharge(47, 1, data.charge);
  drawEucSpeed(0, 18, data.speed);
  drawEucPwm(72, 18, data.pwm);
}

void screenNoLinkDraw(uint8_t dev_charge, bool is_charging)
{
  drawDevCharge(0, 1, dev_charge, is_charging);
  oled.setCursor(10, 3);
  oled.setScale(2);
  oled.print("нет связи");
}
