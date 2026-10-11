#pragma once
#include "euc_data.h"

// сетка из четырёх значений (EUC_VALUE_*), по строкам: слева сверху, справа сверху, слева снизу, справа снизу.
// Связь с телефоном вынесена на экран связи и версий (screen_info)
void screenGridDraw(euc_data_t& data, const uint8_t values[4]);
