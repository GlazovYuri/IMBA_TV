#include "display.h"
#include "GyverOLED.h"
#include "logo.h"
#include "screen_main.h"
#include "screen_grid.h"

static const uint32_t blink_period_low = 1000; //ms
static const uint32_t blink_period_high = 300; //ms
static const uint32_t blink_length = 50; //ms

static const int scr_width = 128;

// режимы экрана, переключаются коротким нажатием кнопки
enum
{
  MODE_CHARGE,   // основной экран, заряд колеса в процентах (screen_main)
  MODE_VOLTAGE,  // основной экран, напряжение колеса (screen_main)
  MODE_GRID,     // сетка параметров и связь (screen_grid)
  display_modes
};

static uint8_t display_mode = MODE_CHARGE;

// индикатор страниц: точки в правом нижнем углу, видны после нажатия кнопки
static const uint32_t mode_dots_show_time = 1500; //ms
static const int mode_dot_size = 3;
static const int mode_dot_pitch = 5;
static bool mode_dots_visible = false;
static uint32_t mode_dots_time = 0;

GyverOLED<SSH1106_128x64> oled;

void displayInit()
{
  oled.init();
  Wire.setClock(400000);
}

void displayOff()
{
  Wire.end();
  pinMode(PIN_WIRE_SCL, INPUT);
  pinMode(PIN_WIRE_SDA, INPUT);
}

void displaySetBrightness(uint8_t val)
{
  oled.setContrast(val);
}

void displayDrawLogo()
{
  oled.clear();
  oled.drawBitmap(logo_x, logo_y, logo_bitmap, logo_width, logo_height);
  oled.update();
}

void displayNextMode()
{
  display_mode = (display_mode + 1) % display_modes;
  mode_dots_visible = true;
  mode_dots_time = millis();
}

// точки в правом нижнем углу: выбранная страница залита, остальные контуром, под точками чёрная подложка
static void drawModeDots()
{
  if(!mode_dots_visible) return;
  if(millis() - mode_dots_time > mode_dots_show_time)
  {
    mode_dots_visible = false;
    return;
  }

  int width = display_modes * mode_dot_pitch - (mode_dot_pitch - mode_dot_size);
  int x0 = scr_width - width;
  int y = 64 - mode_dot_size;
  oled.rect(x0 - 1, y - 1, scr_width - 1, 63, OLED_CLEAR);
  for(int i = 0; i < display_modes; i++)
  {
    int x = x0 + i * mode_dot_pitch;
    oled.rect(x, y, x + mode_dot_size - 1, 63, (i == display_mode) ? OLED_FILL : OLED_STROKE);
  }
}

void displayRenderIface(uint8_t dev_charge, bool is_charging, euc_data_t& data)
{
  oled.clear();

  if(!data.is_connected) screenNoLinkDraw(dev_charge, is_charging);
  else if(display_mode == MODE_CHARGE) screenMainDraw(dev_charge, is_charging, data, false);
  else if(display_mode == MODE_VOLTAGE) screenMainDraw(dev_charge, is_charging, data, true);
  else screenGridDraw(data);

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

  if(data.pwm >= data.pwm_limit)
  {
    uint32_t period = map(data.pwm, data.pwm_limit, 100,
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
