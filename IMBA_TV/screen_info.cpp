#include "screen_info.h"
#include "screen_text.h"
#include "utils.h"
#include "version.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

static const int scr_width = 128;
// кадров от телефона нет дольше этого времени - связь показывается прочерками
static const uint32_t link_stall_ms = 3000;

// строка мелким шрифтом: подпись у левого края, значение у правого
static void drawRow(int y, const char* label, const char* value)
{
  oled.setCursorXY(1, y);
  oled.print(label);

  // знак 6 px; в UTF-8 считаем только первые байты символов
  int chars = 0;
  for(const char* p = value; *p; p++) if((*p & 0xC0) != 0x80) chars++;
  oled.setCursorXY(scr_width - chars * 6, y);
  oled.print(value);
}

void screenInfoDraw(euc_data_t& data)
{
  // крупные числа в тех же столбцах, что на сетке: левое до 3 знаков, правое до 5 (с точкой)
  const int left_right = 36;
  const int left_unit = 39;
  const int right_right = 112;
  const int right_unit = 115;
  const int label_y = 0;
  const int value_y = 10;
  const int separator_y = 31;
  const int firmware_y = 37;
  const int bootloader_y = 50;
  char text[12];

  // задержка: среднее время между кадрами от телефона, "--" если кадров нет дольше link_stall_ms
  bool link_ok = data.is_connected && data.frame_interval_ms != 0 && (millis() - data.last_frame_ms) < link_stall_ms;

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
  screenTextBig(left_right, value_y, text);
  screenTextUnit(left_unit, value_y, interval_unit);

  // частота прихода кадров
  if(link_ok)
  {
    uint32_t hz_x10 = (10000UL + data.frame_interval_ms / 2) / data.frame_interval_ms;
    snprintf(text, sizeof(text), "%lu.%lu", (unsigned long)(hz_x10 / 10), (unsigned long)(hz_x10 % 10));
  }
  else snprintf(text, sizeof(text), "--.-");
  screenTextBig(right_right, value_y, text);
  screenTextUnit(right_unit, value_y, "Гц");

  // подписи и версии - после крупного текста и с наложением: он пишет в экран целыми байтами и стёр бы их
  oled.textMode(BUF_ADD);
  oled.setScale(1);
  oled.setCursorXY(1, label_y);
  oled.print("Задержка");
  oled.setCursorXY(66, label_y);
  oled.print("Частота");

  drawRow(firmware_y, "Прошивка", FW_VERSION);
  const char* bootloader = bootloaderVersionText();
  drawRow(bootloader_y, "Загрузчик", bootloader[0] ? bootloader : "--");
  oled.textMode(BUF_REPLACE);

  oled.fastLineH(separator_y, 0, scr_width - 1);
}
