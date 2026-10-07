#include "battery.h"
#include <Arduino.h>

static const uint16_t volt_critical = 3000; //mV
static const uint16_t volt_low = 3300; //mV
static const uint16_t volt_high = 4200; //mV

static float adc_filt = 0.0f;
static uint16_t volt = 0; //mV

void batteryInit()
{
  analogReference(AR_INTERNAL_1_2);
  analogSampleTime(40);
  adc_filt = analogReadVDDHDIV5();
  batteryUpdate();
}

void batteryUpdate()
{
  uint16_t adc = analogReadVDDHDIV5();
  adc_filt += ((float)adc - adc_filt) * 0.01f;
  volt = adc_filt * (5.0f * 1200.0f / 1024.0f);
}

bool batteryIsChargeCritical()
{
  return volt < volt_critical;
}

uint8_t batteryGetCharge()
{
  int32_t tmp = map(volt, volt_low, volt_high, 0, 100);
  return constrain(tmp, 0, 100);
}
