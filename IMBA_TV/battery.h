#pragma once
#include <stdint.h>

void batteryInit();
void batteryUpdate();
bool batteryIsChargeCritical();
uint8_t batteryGetCharge();
