#include "display.h"
#include "config.h"
#include "GyverOLED.h"
#include "logo.h"
#include "qr.h"
#include "screen_main.h"
#include "screen_grid.h"
#include "screen_info.h"
#include "euc_values.h"

static const uint32_t blink_period_low = 1000; //ms
static const uint32_t blink_period_high = 300; //ms
static const uint32_t blink_length = 50; //ms

static const int scr_width = 128;

// экраны из настроек по порядку, переключаются коротким нажатием кнопки.
// type - SCREEN_*, v - значения EUC_VALUE_*: у экрана с двумя значениями верхняя строка, слева и справа,
// у сетки клетки по строкам
struct screen_t
{
  uint8_t type;
  uint8_t v[4];
};

static const uint8_t display_modes = 5;
static screen_t modes[display_modes];
static uint8_t modes_cnt = 0;
static uint8_t display_mode = 0;

static uint8_t contrast = 255;

// индикатор страниц: точки в правом нижнем углу, видны после нажатия кнопки.
// У каждой страницы место 6×6 с точкой 2×2 посередине, у выбранной по краю места рамка
static const uint32_t mode_dots_show_time = 1500; //ms
static const int mode_slot = 6;
static const int mode_dot_size = 2;
static bool mode_dots_visible = false;
static uint32_t mode_dots_time = 0;

// спиннер экрана обновления: 8 позиций по кругу радиусом 6 px, голова 3×3, за ней 2×2 и точка
static const uint32_t spinner_step = 100; //ms
static const int8_t spinner_dx[8] = {6, 4, 0, -4, -6, -4, 0, 4};
static const int8_t spinner_dy[8] = {0, 4, 6, 4, 0, -4, -6, -4};

GyverOLED<SSH1106_128x64> oled;

static bool isBigValue(uint8_t value)
{
  return value == EUC_VALUE_SPEED || value == EUC_VALUE_PWM || value == EUC_VALUE_TEMPERATURE;
}

void displayInit()
{
  const fw_config_t& cfg = config();
#define SCREEN(n) { cfg.screen##n##_type, { cfg.screen##n##_v1, cfg.screen##n##_v2, cfg.screen##n##_v3, cfg.screen##n##_v4 } }
  const screen_t slots[display_modes] = { SCREEN(1), SCREEN(2), SCREEN(3), SCREEN(4), SCREEN(5) };
#undef SCREEN

  modes_cnt = 0;
  for(uint8_t i = 0; i < display_modes; i++)
  {
    screen_t s = slots[i];
    if(s.type == SCREEN_NONE) continue;
    // у экрана с двумя значениями свои варианты: сверху заряд или напряжение, крупно скорость, ШИМ или температура
    if(s.type == SCREEN_MAIN)
    {
      if(s.v[0] != EUC_VALUE_VOLTAGE) s.v[0] = EUC_VALUE_CHARGE;
      if(!isBigValue(s.v[1])) s.v[1] = EUC_VALUE_SPEED;
      if(!isBigValue(s.v[2])) s.v[2] = EUC_VALUE_PWM;
    }
    modes[modes_cnt++] = s;
  }
  if(modes_cnt == 0) modes[modes_cnt++] = { SCREEN_MAIN, { EUC_VALUE_CHARGE, EUC_VALUE_SPEED, EUC_VALUE_PWM, EUC_VALUE_TRIP } };
  display_mode = 0;

  oled.init();
  Wire.setClock(400000);
}

void displayOff()
{
  Wire.end();
  pinMode(PIN_WIRE_SCL, INPUT);
  pinMode(PIN_WIRE_SDA, INPUT);
}

void displaySetBrightness(uint8_t percent)
{
  contrast = (uint16_t)constrain(percent, 0, 100) * 255 / 100;
  oled.setContrast(contrast);
}

uint8_t displayGetContrast()
{
  return contrast;
}

void displayDrawLogo()
{
  oled.clear();
  oled.drawBitmap(logo_x, logo_y, logo_bitmap, logo_width, logo_height);
  oled.update();
}

void displayNextMode()
{
  if(modes_cnt < 2) return;
  display_mode = (display_mode + 1) % modes_cnt;
  mode_dots_visible = true;
  mode_dots_time = millis();
}

// точки в правом нижнем углу: выбранная страница в рамке, под индикатором чёрная подложка.
// Высота 6 px: на экране версий индикатор помещается под последней строкой
static void drawModeDots()
{
  if(!mode_dots_visible) return;
  if(millis() - mode_dots_time > mode_dots_show_time)
  {
    mode_dots_visible = false;
    return;
  }

  int x0 = scr_width - modes_cnt * mode_slot;
  int y0 = 64 - mode_slot;
  int dot = (mode_slot - mode_dot_size) / 2;
  oled.rect(x0 - 1, y0 - 1, scr_width - 1, 63, OLED_CLEAR);
  for(int i = 0; i < modes_cnt; i++)
  {
    int x = x0 + i * mode_slot;
    oled.rect(x + dot, y0 + dot, x + dot + mode_dot_size - 1, y0 + dot + mode_dot_size - 1, OLED_FILL);
    if(i == display_mode) oled.rect(x, y0, x + mode_slot - 1, 63, OLED_STROKE);
  }
}

void displayRenderIface(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  oled.clear();

  const screen_t& screen = modes[display_mode];
  // версии полезны и без телефона, поэтому экран связи и версий показывается всегда
  if(screen.type == SCREEN_INFO) screenInfoDraw(data);
  else if(!data.is_connected) screenNoLinkDraw(dev_charge, is_charging);
  else if(screen.type == SCREEN_GRID) screenGridDraw(data, screen.v);
  else screenMainDraw(dev_charge, is_charging, data, screen.v[0], screen.v[1], screen.v[2]);

  drawModeDots();
}

void displayDrawIface(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  displayRenderIface(dev_charge, is_charging, data);
  oled.update();
}

void displayUpdateAlarm(euc_data_t& data)
{
  static uint32_t blink_prev = 0;
  static bool inverted = 0;
  uint32_t time = millis();

  const fw_config_t& cfg = config();
  uint8_t limit = (cfg.pwm_alarm_source == CONFIG_PWM_ALARM_SOURCE_FIXED) ? cfg.pwm_alarm_threshold : data.pwm_limit;

  if(cfg.pwm_alarm && data.pwm >= limit)
  {
    uint32_t period = map(data.pwm, limit, 100,
      blink_period_low, blink_period_high);

    if((time - blink_prev) > period)
    {
      blink_prev = time;
      inverted = true;
      oled.invertDisplay(true);
    }
  }

  if(inverted && (time - blink_prev) > blink_length)
  {
    inverted = false;
    oled.invertDisplay(false);
  }
}

void displayClear()
{
  oled.invertDisplay(false);
  oled.clear();
  oled.update();
}

// Удержание кнопки после включения: полоса заполняется, по её окончании включается режим обновления
void displayDrawUpdateHold(uint8_t percent)
{
  oled.clear();
  // строки ближе 8 px: текст по умолчанию пишется целыми байтами и стёр бы соседнюю строку
  oled.textMode(BUF_ADD);
  oled.setScale(1);
  oled.setCursorXY(4, 4);
  oled.print("Держите кнопку,");
  oled.setCursorXY(4, 14);
  oled.print("чтобы включить");
  oled.setCursorXY(4, 24);
  oled.print("обновление по");
  oled.setCursorXY(4, 34);
  oled.print("Bluetooth");
  oled.textMode(BUF_REPLACE);

  int fill = (scr_width - 10) * constrain(percent, 0, 100) / 100;
  oled.rect(4, 48, scr_width - 5, 57, OLED_STROKE);
  if(fill > 0) oled.rect(5, 49, 4 + fill, 56, OLED_FILL);
  oled.update();
}

static void drawSpinner(int cx, int cy)
{
  uint8_t head = (millis() / spinner_step) % 8;
  for(uint8_t k = 0; k < 3; k++)
  {
    uint8_t i = (head + 8 - k) % 8;
    int x = cx + spinner_dx[i];
    int y = cy + spinner_dy[i];
    if(k == 0) oled.rect(x - 1, y - 1, x + 1, y + 1, OLED_FILL);
    else if(k == 1) oled.rect(x - 1, y - 1, x, y, OLED_FILL);
    else oled.dot(x, y);
  }
}

// Экран режима обновления: пока ждём, QR-код сайта, после подключения надпись об этом.
// Сайт подключается, только когда нажали «Прошить», и сразу переводит плату в загрузчик.
// Загрузчик дисплей не обновляет, и последний кадр остаётся на экране всю прошивку
void displayDrawUpdateMode(bool connected)
{
  oled.clear();
  // строки ближе 8 px: текст по умолчанию пишется целыми байтами и стёр бы соседнюю строку
  oled.textMode(BUF_ADD);
  oled.setScale(1);
  if(connected)
  {
    oled.setScale(2);
    oled.setCursorXY(4, 0);
    oled.print("Обновление");
    oled.setScale(1);
    oled.setCursorXY(4, 18);
    oled.print("по Bluetooth");
    // центр пустого места справа от текста: x 75..127 (до края экрана), y 14..54
    drawSpinner(101, 34);
    oled.setCursorXY(4, 36);
    oled.print("Устройство");
    oled.setCursorXY(4, 45);
    oled.print("подключено");
    oled.setCursorXY(4, 54);
    oled.print("Прогресс на сайте");
  }
  else
  {
    // справа от кода 62 px, это 10 символов в строке
    oled.drawBitmap(0, 0, qr_bitmap, qr_size, qr_size);
    oled.setCursorXY(66, 1);
    oled.print("Обновление");
    oled.setCursorXY(66, 10);
    oled.print("Bluetooth");
    drawSpinner(96, 27);
    oled.setCursorXY(66, 40);
    oled.print("Выкл:");
    oled.setCursorXY(66, 48);
    oled.print("держать");
    oled.setCursorXY(66, 56);
    oled.print("кнопку");
  }
  oled.textMode(BUF_REPLACE);
  oled.update();
}

// Режим обновления не включился: заряда мало для прошивки по Bluetooth
void displayDrawUpdateLowBattery(uint8_t charge, uint8_t min_charge)
{
  char text[40];  // кириллица в UTF-8 — 2 байта на букву
  oled.clear();
  oled.textMode(BUF_ADD);
  oled.setScale(2);
  oled.setCursorXY(4, 0);
  oled.print("Зарядите");
  oled.setCursorXY(4, 17);
  oled.print("дисплей");
  oled.setScale(1);
  oled.setCursorXY(4, 38);
  oled.print("Для обновления по");
  oled.setCursorXY(4, 47);
  snprintf(text, sizeof(text), "Bluetooth нужно %u%%", min_charge);
  oled.print(text);
  oled.setCursorXY(4, 56);
  snprintf(text, sizeof(text), "Сейчас: %u%%", charge);
  oled.print(text);
  oled.textMode(BUF_REPLACE);
  oled.update();
}
