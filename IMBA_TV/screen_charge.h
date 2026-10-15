#pragma once
#include <stdint.h>
#include "euc_data.h"
#include "snapshot.h"

// Экран зарядки колеса: посередине молния, залитая снизу на заряд колеса, под ней проценты.
// Сверху напряжение и ток зарядки, снизу температура самой горячей точки батареи и зарядника.
// Оформлений три, у каждого своё появление и своя анимация молнии

static const uint8_t charge_styles = 3;

// экран открылся: появление начинается заново в оформлении style (0..charge_styles-1)
void screenChargeOpen(uint8_t style);

void screenChargeDraw(const euc_data_t& data, const snapshot_charge_t& charge);
