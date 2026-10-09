#pragma once
#include <stdint.h>
#include "euc_data.h"

void displayPowerOffAnimation(uint8_t dev_charge, bool is_charging, euc_data_t& data);
void displayPlayIntroNum(int anim, uint32_t seed, uint8_t dev_charge, bool is_charging, euc_data_t& data);
void displayPlayIntro(uint8_t dev_charge, bool is_charging, euc_data_t& data);
