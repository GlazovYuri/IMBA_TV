#include "utils.h"
#include <bluefruit.h>
#include <Arduino.h>

static const uint8_t button_pin = 0;
static const uint32_t on_delay = 500; //ms
static const uint32_t off_delay = 1000; //ms
static const uint32_t button_period = 20; //ms

static bool button_hold = false;

void softDeviceInit()
{
  Bluefruit.begin();
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

static bool buttonState()
{
  return !digitalRead(button_pin);
}

bool buttonWaitPowerup()
{
  uint16_t cnt = on_delay / button_period;
  while(cnt--)
  {
    if(!buttonState()) return false;
    delay(button_period);
  }

  button_hold = true;
  return true;
}

bool buttonIsLongPress()
{
  bool state = buttonState();
  if(button_hold)
  {
    if(!state) button_hold = false;
    return false;
  }

  static uint32_t unpressed_time = 0;
  uint32_t time = millis();
  if(state)
  {
    if(time - unpressed_time > off_delay)
    {
      return true;
    }
  }
  else
  {
    unpressed_time = time;
  }

  return false;
}

void buttonWaitRelease()
{
  while(buttonState()) delay(button_period);
}
