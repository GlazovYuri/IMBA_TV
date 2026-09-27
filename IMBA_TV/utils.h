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
bool buttonIsLongPress();
void buttonWaitRelease();
