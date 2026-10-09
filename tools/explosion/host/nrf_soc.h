#pragma once
#include <stdint.h>
inline uint32_t sd_softdevice_is_enabled(uint8_t* p) { *p = 0; return 0; }
inline uint32_t sd_rand_application_vector_get(uint8_t*, uint8_t) { return 0; }
