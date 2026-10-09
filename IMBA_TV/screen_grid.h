#pragma once
#include "euc_data.h"

// сетка (режим 3): заряд, напряжение, температура, пробег.
// Связь с телефоном вынесена на экран связи и версий (screen_info)
void screenGridDraw(euc_data_t& data);
