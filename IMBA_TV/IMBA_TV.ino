#include <Adafruit_TinyUSB.h>
#include "config.h"
#include "utils.h"
#include "battery.h"
#include "ble.h"
#include "display.h"
#include "animations.h"
#include "update_mode.h"

static const uint32_t update_period = 40; //ms

void setup()
{
    configLoad();
    softDeviceInit();

    buttonInitSense();
    usbDetectEnable();
    batteryInit();

    bool by_button = false;
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
        by_button = true;
    }

    powerOn();
    delay(100);
    displayInit();
    displaySetBrightness(config().brightness);

    // кнопку держат и после включения экрана - режим обновления по Bluetooth
    if(by_button && updateModeRequested())
    {
        bleInit(true);
        bleAdvertise(true);
        updateModeRun();
    }

    bleInit();
    bleAdvertise();

    displayPlayIntro(batteryGetCharge(), isUsbConnected(), bleGetEucData());
}

void loop()
{
    button_event_t button = buttonPoll();
    if(button == BUTTON_SHORT) displayNextMode();
    if(button == BUTTON_LONG)
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
