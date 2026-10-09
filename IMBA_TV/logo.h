#pragma once
#include <stdint.h>

const int logo_x = 0;
const int logo_y = 0;
const int logo_width = 128;
const int logo_height = 64;

// определение в logo.cpp, чтобы массив был во флеше в одном экземпляре
extern const uint8_t logo_bitmap[logo_width * logo_height / 8];
