#pragma once
#include <stdint.h>
// SoftDevice на компьютере выключен: зерно заставки задаёт стенд
static inline uint32_t sd_softdevice_is_enabled(uint8_t* on) { *on = 0; return 0; }
static inline uint32_t sd_rand_application_vector_get(uint8_t*, uint8_t) { return 0; }
