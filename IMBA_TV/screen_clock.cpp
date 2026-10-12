#include "screen_clock.h"
#include "snapshot.h"
#include "config.h"
#include "GyverOLED.h"

extern GyverOLED<SSH1106_128x64> oled;

static const int scr_width = 128;

// время крупно (scale 4): знак 20 px и промежуток 4 px. Двоеточие узкое, две точки 4×4,
// от цифр по 6 px, так «12:34» занимает 104 px и стоит посередине
static const int time_x = 12;
static const int time_y = 6;
static const int minutes_x = 72;
static const int colon_x = 62;
static const int colon_size = 4;
static const int colon_top = 7;      // от верха цифр
static const int colon_bottom = 17;
// дата (scale 2): знак 10 px и промежуток 2 px
static const int date_y = 44;

static const char* const weekdays[7] = {"пн", "вт", "ср", "чт", "пт", "сб", "вс"};

// строка крупным шрифтом по центру экрана; в UTF-8 считаем только первые байты символов
static void printCentered(int y, const char* text)
{
  int chars = 0;
  for(const char* p = text; *p; p++) if((*p & 0xC0) != 0x80) chars++;
  oled.setScale(2);
  oled.setCursorXY((scr_width - (chars * 12 - 2)) / 2, y);
  oled.print(text);
}

void screenClockDraw()
{
  char hours[3] = "--";
  char minutes[3] = "--";
  char date[16] = "--.--";

  uint64_t unix_ms;
  if(snapshotTimeMs(unix_ms))
  {
    // пояс в настройках - сдвиг от UTC в четвертях часа плюс 48. Метка не раньше 2020 года,
    // поэтому вычитание не уходит в минус
    uint64_t local_s = unix_ms / 1000 + config().timezone * 900ULL - 48 * 900ULL;
    uint32_t secs = local_s % 86400;
    uint32_t days = local_s / 86400;
    snprintf(hours, sizeof(hours), "%02u", (unsigned)(secs / 3600));
    snprintf(minutes, sizeof(minutes), "%02u", (unsigned)(secs / 60 % 60));

    // число и месяц по дням от 1970-01-01 (алгоритм civil_from_days Говарда Хиннанта), это был четверг
    uint32_t z = days + 719468;
    uint32_t era = z / 146097;
    uint32_t doe = z - era * 146097;
    uint32_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    uint32_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    uint32_t mp = (5 * doy + 2) / 153;
    uint32_t day = doy - (153 * mp + 2) / 5 + 1;
    uint32_t month = mp < 10 ? mp + 3 : mp - 9;
    snprintf(date, sizeof(date), "%s %02u.%02u", weekdays[(days + 3) % 7], (unsigned)day, (unsigned)month);
  }

  oled.setScale(4);
  oled.setCursorXY(time_x, time_y);
  oled.print(hours);
  oled.setCursorXY(minutes_x, time_y);
  oled.print(minutes);
  oled.rect(colon_x, time_y + colon_top, colon_x + colon_size - 1, time_y + colon_top + colon_size - 1, OLED_FILL);
  oled.rect(colon_x, time_y + colon_bottom, colon_x + colon_size - 1, time_y + colon_bottom + colon_size - 1, OLED_FILL);

  printCentered(date_y, date);
}
