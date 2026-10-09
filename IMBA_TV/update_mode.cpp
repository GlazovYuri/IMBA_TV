#include "update_mode.h"
#include "config.h"
#include "utils.h"
#include "battery.h"
#include "ble.h"
#include "display.h"
#include <Arduino.h>

// вызывается, когда кнопку не отпускали всю секунду показа логотипа, поэтому полоса сразу
static const uint32_t hold_time = 3000; //ms
static const uint32_t idle_timeout = 10UL * 60 * 1000; //ms
static const uint32_t update_period = 100; //ms
// Загрузчик стирает старую прошивку в начале обновления. Если батарея сядет посреди
// передачи, дисплей будет ждать новую прошивку и не включится, поэтому без USB
// режим обновления включается только при достаточном заряде
static const uint8_t min_charge = 30; //%
static const uint32_t low_battery_show_time = 3000; //ms

bool updateModeRequested()
{
  if(!config().ble_update) return false;

  uint32_t start = millis();
  while(buttonPressed())
  {
    uint32_t t = millis() - start;
    if(t >= hold_time)
    {
      uint8_t charge = batteryGetCharge();
      if(charge >= min_charge || isUsbConnected()) return true;

      // заряда мало: объясняем и включаемся как обычно
      displayDrawUpdateLowBattery(charge, min_charge);
      delay(low_battery_show_time);
      return false;
    }
    displayDrawUpdateHold(t * 100 / hold_time);
    delay(20);
  }
  return false;
}

static void updateModeExit()
{
  displayClear();
  displayOff();
  powerOff();
  buttonWaitRelease();
  systemOff();
}

void updateModeRun()
{
  uint32_t start = millis();
  for(;;)
  {
    displayDrawUpdateMode(bleGetEucData().is_connected);

    batteryUpdate();
    if(buttonPoll() == BUTTON_LONG) updateModeExit();
    if(batteryIsChargeCritical() && !isUsbConnected()) updateModeExit();
    if(millis() - start > idle_timeout) updateModeExit();

    delay(update_period);
  }
}
