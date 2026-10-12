#pragma once
#include <stdint.h>
#include "euc_data.h"

// экран с двумя значениями: сверху заряд устройства и колеса, крупно два значения.
// top: EUC_VALUE_CHARGE - заряд колеса в процентах, EUC_VALUE_VOLTAGE - напряжение колеса,
// EUC_VALUE_CLOCK - часы посередине и заряд колеса в правом углу;
// left, right: скорость, ШИМ или температура (EUC_VALUE_*)
void screenMainDraw(uint8_t dev_charge, bool is_charging, euc_data_t& data, uint8_t top, uint8_t left, uint8_t right);

// нет связи с телефоном: заряд устройства и надпись
void screenNoLinkDraw(uint8_t dev_charge, bool is_charging);
