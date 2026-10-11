#pragma once
#include <stdint.h>

// вызовы SoftDevice (S140), которые использует прошивка
#define NRF_SUCCESS 0
#define NRF_ERROR_SOFTDEVICE_NOT_ENABLED 0x2001
#define NRF_ERROR_INVALID_LENGTH 9

#define POWER_RESETREAS_RESETPIN_Msk (1UL << 0)
#define POWER_RESETREAS_DOG_Msk (1UL << 1)
#define POWER_RESETREAS_SREQ_Msk (1UL << 2)
#define POWER_RESETREAS_LOCKUP_Msk (1UL << 3)
#define POWER_RESETREAS_OFF_Msk (1UL << 16)
#define POWER_RESETREAS_VBUS_Msk (1UL << 20)

#define POWER_USBREGSTATUS_VBUSDETECT_Msk (1UL << 0)
#define POWER_USBREGSTATUS_OUTPUTRDY_Msk (1UL << 1)

uint32_t sd_softdevice_is_enabled(uint8_t* enabled);
uint32_t sd_power_system_off();
uint32_t sd_power_usbdetected_enable(uint8_t enable);
uint32_t sd_power_usbregstatus_get(uint32_t* status);
uint32_t sd_rand_application_vector_get(uint8_t* buf, uint8_t len);
