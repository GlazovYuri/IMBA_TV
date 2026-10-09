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

    // логотип сразу после включения экрана, иначе до первой картинки виден мусор из памяти дисплея.
    // Режим обновления по Bluetooth: кнопку не отпустили, пока логотип висел, и держат дальше
    bool held = displayIntroBegin(by_button, batteryGetCharge(), isUsbConnected(), bleGetEucData());
    if(held && updateModeRequested())
    {
        bleInit(true);
        bleAdvertise(true);
        updateModeRun();
    }

    bleInit();
    bleAdvertise();

    displayIntroFinish(batteryGetCharge(), isUsbConnected(), bleGetEucData());
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
