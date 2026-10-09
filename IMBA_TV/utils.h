#pragma once

void softDeviceInit();

void powerOn();
void powerOff();
void systemOff();

void usbDetectEnable();
bool isUsbWakeup();
bool isUsbConnected();

void buttonInitSense();
void buttonInitNoSense();
bool buttonWaitPowerup();
enum button_event_t
{
  BUTTON_NONE,
  BUTTON_SHORT,  // отпущена раньше off_delay
  BUTTON_LONG,   // удерживается дольше off_delay
};

button_event_t buttonPoll();
void buttonWaitRelease();
