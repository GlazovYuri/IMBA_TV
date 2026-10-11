#pragma once
#include <stdint.h>
#include "euc_data.h"

void displayPowerOffAnimation(uint8_t dev_charge, bool is_charging, euc_data_t& data);
void displayPlayIntroNum(int anim, uint32_t seed, uint8_t dev_charge, bool is_charging, euc_data_t& data);

// Начало заставки сразу после включения экрана: логотип (без заставки - интерфейс) на intro_logo_hold.
// true, если watch_button и кнопку всё это время не отпускали
bool displayIntroBegin(bool watch_button, uint8_t dev_charge, bool is_charging, euc_data_t& data);
// Анимация перехода от логотипа к интерфейсу (по настройке заставки)
void displayIntroFinish(uint8_t dev_charge, bool is_charging, euc_data_t& data);
