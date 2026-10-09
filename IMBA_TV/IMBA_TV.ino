#include <Adafruit_TinyUSB.h>
#include "utils.h"
#include "battery.h"
#include "ble.h"
#include "display.h"
#include "animations.h"

static const uint8_t brightness = 255;
static const uint32_t update_period = 40; //ms

void setup()
{
    softDeviceInit();

    buttonInitSense();
    usbDetectEnable();
    batteryInit();

    if(isUsbWakeup())
    {
        if(!isUsbConnected()) systemOff();
    }
    else
    {
        if(batteryIsChargeCritical())
        {
            buttonInitNoSense();
            systemOff();
        }

        if(!buttonWaitPowerup()) systemOff();
    }

    bleInit();
    bleAdvertise();

    powerOn();
    delay(100);
    displayInit();
    displaySetBrightness(brightness);

    displayPlayIntro(batteryGetCharge(), isUsbConnected(), bleGetEucData());
}

void loop()
{
    if(buttonIsLongPress())
    {
        displayPowerOffAnimation(batteryGetCharge(), isUsbConnected(), bleGetEucData());
        displayOff();
        powerOff();
        buttonWaitRelease();
        systemOff();
    }

    bool usb_connected = isUsbConnected();

    batteryUpdate();
    if(batteryIsChargeCritical() && !usb_connected)
    {
        buttonInitNoSense();
        displayPowerOffAnimation(batteryGetCharge(), usb_connected, bleGetEucData());
        displayOff();
        powerOff();
        systemOff();
    }

    euc_data_t& data = bleGetEucData();
    displayDrawIface(batteryGetCharge(), usb_connected, data);
    displayUpdateAlarm(data);

    delay(update_period);
}
