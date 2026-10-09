#pragma once
#include <stdint.h>
#include "euc_data.h"

// основной экран (режимы 1 и 2): сверху заряд устройства и колеса, крупно скорость и ШИМ
// show_voltage = false: заряд колеса в процентах, true: напряжение колеса
void screenMainDraw(uint8_t dev_charge, bool is_charging, euc_data_t& data, bool show_voltage);

// нет связи с телефоном: заряд устройства и надпись
void screenNoLinkDraw(uint8_t dev_charge, bool is_charging);
