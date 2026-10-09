#include "update_mode.h"
#include "config.h"
#include "utils.h"
#include "battery.h"
#include "ble.h"
#include "display.h"
#include <Arduino.h>

// кнопку часто отпускают чуть позже, чем загорается экран: подсказку показываем не сразу
static const uint32_t hold_grace = 400; //ms
static const uint32_t hold_time = 3000; //ms
static const uint32_t idle_timeout = 10UL * 60 * 1000; //ms
static const uint32_t update_period = 100; //ms

bool updateModeRequested()
{
  if(!config().ble_update) return false;

  uint32_t start = millis();
  while(buttonPressed())
  {
    uint32_t t = millis() - start;
    if(t >= hold_grace + hold_time) return true;
    if(t >= hold_grace) displayDrawUpdateHold((t - hold_grace) * 100 / hold_time);
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
