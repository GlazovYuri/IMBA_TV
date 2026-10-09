#include "screen_text.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

void screenTextBig(int right, int y, const char* text)
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

void screenTextUnit(int x, int y, const char* unit)
{
  oled.setScale(1);
  oled.setCursorXY(x, y + 7);
  oled.print(unit);
}
