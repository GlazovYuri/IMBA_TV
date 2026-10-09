#include "utils.h"
#include "config.h"
#include <bluefruit.h>
#include <Arduino.h>

static const uint8_t button_pin = 0;
static const uint32_t button_period = 20; //ms

static bool button_hold = false;

void softDeviceInit()
{
  Bluefruit.begin();
}

const char* bootloaderVersionText()
{
  // ядро читает версию, которую загрузчик оставляет в регистре, как 0x00MMmmpp
  static char text[12] = "";
  uint32_t v = bootloaderVersion;
  if(!text[0] && v != 0 && v <= 0xFFFFFF)
  {
    snprintf(text, sizeof(text), "%lu.%lu.%lu",
             (unsigned long)(v >> 16), (unsigned long)((v >> 8) & 0xFF), (unsigned long)(v & 0xFF));
  }
  return text;
}

void powerOn()
{
  pinMode(PIN_PWREN, OUTPUT);
  digitalWrite(PIN_PWREN, HIGH);
}

void powerOff()
{
  digitalWrite(PIN_PWREN, LOW);
}

void systemOff()
{
  sd_power_system_off();
}

void usbDetectEnable()
{
  sd_power_usbdetected_enable(1);
}

bool isUsbWakeup()
{
  return readResetReason() & POWER_RESETREAS_VBUS_Msk;
}

bool isUsbConnected()
{
  uint32_t usbregstatus = 0;
  sd_power_usbregstatus_get(&usbregstatus);
  return usbregstatus & POWER_USBREGSTATUS_VBUSDETECT_Msk;
}

void buttonInitSense()
{
  pinMode(button_pin, INPUT_PULLUP_SENSE);
}

void buttonInitNoSense()
{
  pinMode(button_pin, INPUT_PULLUP);
}

bool buttonPressed()
{
  return !digitalRead(button_pin);
}

bool buttonWaitPowerup()
{
  uint16_t cnt = config().power_on_ms / button_period;
  while(cnt--)
  {
    if(!buttonPressed()) return false;
    delay(button_period);
  }

  button_hold = true;
  return true;
}

button_event_t buttonPoll()
{
  bool state = buttonPressed();
  if(button_hold)
  {
    if(!state) button_hold = false;
    return BUTTON_NONE;
  }

  static bool pressed = false;
  static uint32_t press_time = 0;
  uint32_t time = millis();

  if(state)
  {
    if(!pressed)
    {
      pressed = true;
      press_time = time;
    }
    else if(time - press_time > config().power_off_ms)
    {
      return BUTTON_LONG;
    }
  }
  else if(pressed)
  {
    pressed = false;
    if(time - press_time <= config().power_off_ms) return BUTTON_SHORT;
  }

  return BUTTON_NONE;
}

void buttonWaitRelease()
{
  while(buttonPressed()) delay(button_period);
}
