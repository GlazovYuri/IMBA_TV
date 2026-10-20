#pragma once

// Сгенерировано tools/gen_config.py из config.json, не править руками.
// После изменения config.json запустите: python3 tools/gen_config.py

#include <stdint.h>

// Настройки прошивки. Значения записывает веб-конфигуратор перед прошивкой.
struct __attribute__((packed)) fw_config_t
{
  uint8_t screen1_type;  // Экран 1
  uint8_t screen1_v1;  // Экран 1, поле 1
  uint8_t screen1_v2;  // Экран 1, поле 2
  uint8_t screen1_v3;  // Экран 1, поле 3
  uint8_t screen1_v4;  // Экран 1, поле 4
  uint8_t screen2_type;  // Экран 2
  uint8_t screen2_v1;  // Экран 2, поле 1
  uint8_t screen2_v2;  // Экран 2, поле 2
  uint8_t screen2_v3;  // Экран 2, поле 3
  uint8_t screen2_v4;  // Экран 2, поле 4
  uint8_t screen3_type;  // Экран 3
  uint8_t screen3_v1;  // Экран 3, поле 1
  uint8_t screen3_v2;  // Экран 3, поле 2
  uint8_t screen3_v3;  // Экран 3, поле 3
  uint8_t screen3_v4;  // Экран 3, поле 4
  uint8_t screen4_type;  // Экран 4
  uint8_t screen4_v1;  // Экран 4, поле 1
  uint8_t screen4_v2;  // Экран 4, поле 2
  uint8_t screen4_v3;  // Экран 4, поле 3
  uint8_t screen4_v4;  // Экран 4, поле 4
  uint8_t screen5_type;  // Экран 5
  uint8_t screen5_v1;  // Экран 5, поле 1
  uint8_t screen5_v2;  // Экран 5, поле 2
  uint8_t screen5_v3;  // Экран 5, поле 3
  uint8_t screen5_v4;  // Экран 5, поле 4
  uint16_t wheel_voltage_x10;  // Батарея колеса
  uint8_t timezone;  // Часовой пояс
  uint8_t charge_screen;  // Экран зарядки колеса
  uint8_t pwm_alarm;  // Мигать экраном при превышении порога ШИМ
  uint8_t pwm_alarm_source;  // Порог
  uint8_t pwm_alarm_threshold;  // Свой порог ШИМ
  uint8_t brightness;  // Яркость
  uint8_t intro_logo;  // Логотип при включении
  uint8_t intro_clawd;  // Заставки Clawd
  uint8_t intro_wave;  // Волна
  uint8_t intro_break;  // Разрушение
  uint8_t intro_explosion;  // Взрыв
  uint8_t poweroff_animation;  // Анимация выключения
  uint16_t power_on_ms;  // Удержание для включения
  uint16_t power_off_ms;  // Удержание для выключения
  uint8_t ble_update;  // Разрешить обновление прошивки по Bluetooth
};

static_assert(sizeof(fw_config_t) == 44, "config.json и fw_config_t разошлись");

#define FW_CONFIG_DEFAULTS { 1, 6, 0, 1, 5, 2, 2, 3, 4, 5, 0, 2, 3, 4, 5, 0, 2, 3, 4, 5, 0, 2, 3, 4, 5, 0, 60, 1, 1, 0, 80, 100, 1, 0, 1, 1, 1, 1, 500, 1000, 0 }

enum
{
  CONFIG_SCREEN1_TYPE_NONE = 0,
  CONFIG_SCREEN1_TYPE_MAIN = 1,
  CONFIG_SCREEN1_TYPE_GRID = 2,
  CONFIG_SCREEN1_TYPE_INFO = 3,
  CONFIG_SCREEN1_TYPE_CLOCK = 4,
  CONFIG_SCREEN1_V1_SPEED = 0,
  CONFIG_SCREEN1_V1_PWM = 1,
  CONFIG_SCREEN1_V1_CHARGE = 2,
  CONFIG_SCREEN1_V1_VOLTAGE = 3,
  CONFIG_SCREEN1_V1_TEMPERATURE = 4,
  CONFIG_SCREEN1_V1_TRIP = 5,
  CONFIG_SCREEN1_V1_CLOCK = 6,
  CONFIG_SCREEN1_V2_SPEED = 0,
  CONFIG_SCREEN1_V2_PWM = 1,
  CONFIG_SCREEN1_V2_CHARGE = 2,
  CONFIG_SCREEN1_V2_VOLTAGE = 3,
  CONFIG_SCREEN1_V2_TEMPERATURE = 4,
  CONFIG_SCREEN1_V2_TRIP = 5,
  CONFIG_SCREEN1_V2_CLOCK = 6,
  CONFIG_SCREEN1_V3_SPEED = 0,
  CONFIG_SCREEN1_V3_PWM = 1,
  CONFIG_SCREEN1_V3_CHARGE = 2,
  CONFIG_SCREEN1_V3_VOLTAGE = 3,
  CONFIG_SCREEN1_V3_TEMPERATURE = 4,
  CONFIG_SCREEN1_V3_TRIP = 5,
  CONFIG_SCREEN1_V3_CLOCK = 6,
  CONFIG_SCREEN1_V4_SPEED = 0,
  CONFIG_SCREEN1_V4_PWM = 1,
  CONFIG_SCREEN1_V4_CHARGE = 2,
  CONFIG_SCREEN1_V4_VOLTAGE = 3,
  CONFIG_SCREEN1_V4_TEMPERATURE = 4,
  CONFIG_SCREEN1_V4_TRIP = 5,
  CONFIG_SCREEN1_V4_CLOCK = 6,
  CONFIG_SCREEN2_TYPE_NONE = 0,
  CONFIG_SCREEN2_TYPE_MAIN = 1,
  CONFIG_SCREEN2_TYPE_GRID = 2,
  CONFIG_SCREEN2_TYPE_INFO = 3,
  CONFIG_SCREEN2_TYPE_CLOCK = 4,
  CONFIG_SCREEN2_V1_SPEED = 0,
  CONFIG_SCREEN2_V1_PWM = 1,
  CONFIG_SCREEN2_V1_CHARGE = 2,
  CONFIG_SCREEN2_V1_VOLTAGE = 3,
  CONFIG_SCREEN2_V1_TEMPERATURE = 4,
  CONFIG_SCREEN2_V1_TRIP = 5,
  CONFIG_SCREEN2_V1_CLOCK = 6,
  CONFIG_SCREEN2_V2_SPEED = 0,
  CONFIG_SCREEN2_V2_PWM = 1,
  CONFIG_SCREEN2_V2_CHARGE = 2,
  CONFIG_SCREEN2_V2_VOLTAGE = 3,
  CONFIG_SCREEN2_V2_TEMPERATURE = 4,
  CONFIG_SCREEN2_V2_TRIP = 5,
  CONFIG_SCREEN2_V2_CLOCK = 6,
  CONFIG_SCREEN2_V3_SPEED = 0,
  CONFIG_SCREEN2_V3_PWM = 1,
  CONFIG_SCREEN2_V3_CHARGE = 2,
  CONFIG_SCREEN2_V3_VOLTAGE = 3,
  CONFIG_SCREEN2_V3_TEMPERATURE = 4,
  CONFIG_SCREEN2_V3_TRIP = 5,
  CONFIG_SCREEN2_V3_CLOCK = 6,
  CONFIG_SCREEN2_V4_SPEED = 0,
  CONFIG_SCREEN2_V4_PWM = 1,
  CONFIG_SCREEN2_V4_CHARGE = 2,
  CONFIG_SCREEN2_V4_VOLTAGE = 3,
  CONFIG_SCREEN2_V4_TEMPERATURE = 4,
  CONFIG_SCREEN2_V4_TRIP = 5,
  CONFIG_SCREEN2_V4_CLOCK = 6,
  CONFIG_SCREEN3_TYPE_NONE = 0,
  CONFIG_SCREEN3_TYPE_MAIN = 1,
  CONFIG_SCREEN3_TYPE_GRID = 2,
  CONFIG_SCREEN3_TYPE_INFO = 3,
  CONFIG_SCREEN3_TYPE_CLOCK = 4,
  CONFIG_SCREEN3_V1_SPEED = 0,
  CONFIG_SCREEN3_V1_PWM = 1,
  CONFIG_SCREEN3_V1_CHARGE = 2,
  CONFIG_SCREEN3_V1_VOLTAGE = 3,
  CONFIG_SCREEN3_V1_TEMPERATURE = 4,
  CONFIG_SCREEN3_V1_TRIP = 5,
  CONFIG_SCREEN3_V1_CLOCK = 6,
  CONFIG_SCREEN3_V2_SPEED = 0,
  CONFIG_SCREEN3_V2_PWM = 1,
  CONFIG_SCREEN3_V2_CHARGE = 2,
  CONFIG_SCREEN3_V2_VOLTAGE = 3,
  CONFIG_SCREEN3_V2_TEMPERATURE = 4,
  CONFIG_SCREEN3_V2_TRIP = 5,
  CONFIG_SCREEN3_V2_CLOCK = 6,
  CONFIG_SCREEN3_V3_SPEED = 0,
  CONFIG_SCREEN3_V3_PWM = 1,
  CONFIG_SCREEN3_V3_CHARGE = 2,
  CONFIG_SCREEN3_V3_VOLTAGE = 3,
  CONFIG_SCREEN3_V3_TEMPERATURE = 4,
  CONFIG_SCREEN3_V3_TRIP = 5,
  CONFIG_SCREEN3_V3_CLOCK = 6,
  CONFIG_SCREEN3_V4_SPEED = 0,
  CONFIG_SCREEN3_V4_PWM = 1,
  CONFIG_SCREEN3_V4_CHARGE = 2,
  CONFIG_SCREEN3_V4_VOLTAGE = 3,
  CONFIG_SCREEN3_V4_TEMPERATURE = 4,
  CONFIG_SCREEN3_V4_TRIP = 5,
  CONFIG_SCREEN3_V4_CLOCK = 6,
  CONFIG_SCREEN4_TYPE_NONE = 0,
  CONFIG_SCREEN4_TYPE_MAIN = 1,
  CONFIG_SCREEN4_TYPE_GRID = 2,
  CONFIG_SCREEN4_TYPE_INFO = 3,
  CONFIG_SCREEN4_TYPE_CLOCK = 4,
  CONFIG_SCREEN4_V1_SPEED = 0,
  CONFIG_SCREEN4_V1_PWM = 1,
  CONFIG_SCREEN4_V1_CHARGE = 2,
  CONFIG_SCREEN4_V1_VOLTAGE = 3,
  CONFIG_SCREEN4_V1_TEMPERATURE = 4,
  CONFIG_SCREEN4_V1_TRIP = 5,
  CONFIG_SCREEN4_V1_CLOCK = 6,
  CONFIG_SCREEN4_V2_SPEED = 0,
  CONFIG_SCREEN4_V2_PWM = 1,
  CONFIG_SCREEN4_V2_CHARGE = 2,
  CONFIG_SCREEN4_V2_VOLTAGE = 3,
  CONFIG_SCREEN4_V2_TEMPERATURE = 4,
  CONFIG_SCREEN4_V2_TRIP = 5,
  CONFIG_SCREEN4_V2_CLOCK = 6,
  CONFIG_SCREEN4_V3_SPEED = 0,
  CONFIG_SCREEN4_V3_PWM = 1,
  CONFIG_SCREEN4_V3_CHARGE = 2,
  CONFIG_SCREEN4_V3_VOLTAGE = 3,
  CONFIG_SCREEN4_V3_TEMPERATURE = 4,
  CONFIG_SCREEN4_V3_TRIP = 5,
  CONFIG_SCREEN4_V3_CLOCK = 6,
  CONFIG_SCREEN4_V4_SPEED = 0,
  CONFIG_SCREEN4_V4_PWM = 1,
  CONFIG_SCREEN4_V4_CHARGE = 2,
  CONFIG_SCREEN4_V4_VOLTAGE = 3,
  CONFIG_SCREEN4_V4_TEMPERATURE = 4,
  CONFIG_SCREEN4_V4_TRIP = 5,
  CONFIG_SCREEN4_V4_CLOCK = 6,
  CONFIG_SCREEN5_TYPE_NONE = 0,
  CONFIG_SCREEN5_TYPE_MAIN = 1,
  CONFIG_SCREEN5_TYPE_GRID = 2,
  CONFIG_SCREEN5_TYPE_INFO = 3,
  CONFIG_SCREEN5_TYPE_CLOCK = 4,
  CONFIG_SCREEN5_V1_SPEED = 0,
  CONFIG_SCREEN5_V1_PWM = 1,
  CONFIG_SCREEN5_V1_CHARGE = 2,
  CONFIG_SCREEN5_V1_VOLTAGE = 3,
  CONFIG_SCREEN5_V1_TEMPERATURE = 4,
  CONFIG_SCREEN5_V1_TRIP = 5,
  CONFIG_SCREEN5_V1_CLOCK = 6,
  CONFIG_SCREEN5_V2_SPEED = 0,
  CONFIG_SCREEN5_V2_PWM = 1,
  CONFIG_SCREEN5_V2_CHARGE = 2,
  CONFIG_SCREEN5_V2_VOLTAGE = 3,
  CONFIG_SCREEN5_V2_TEMPERATURE = 4,
  CONFIG_SCREEN5_V2_TRIP = 5,
  CONFIG_SCREEN5_V2_CLOCK = 6,
  CONFIG_SCREEN5_V3_SPEED = 0,
  CONFIG_SCREEN5_V3_PWM = 1,
  CONFIG_SCREEN5_V3_CHARGE = 2,
  CONFIG_SCREEN5_V3_VOLTAGE = 3,
  CONFIG_SCREEN5_V3_TEMPERATURE = 4,
  CONFIG_SCREEN5_V3_TRIP = 5,
  CONFIG_SCREEN5_V3_CLOCK = 6,
  CONFIG_SCREEN5_V4_SPEED = 0,
  CONFIG_SCREEN5_V4_PWM = 1,
  CONFIG_SCREEN5_V4_CHARGE = 2,
  CONFIG_SCREEN5_V4_VOLTAGE = 3,
  CONFIG_SCREEN5_V4_TEMPERATURE = 4,
  CONFIG_SCREEN5_V4_TRIP = 5,
  CONFIG_SCREEN5_V4_CLOCK = 6,
  CONFIG_WHEEL_VOLTAGE_X10_AUTO = 0,
  CONFIG_WHEEL_VOLTAGE_X10_20S = 840,
  CONFIG_WHEEL_VOLTAGE_X10_24S = 1008,
  CONFIG_WHEEL_VOLTAGE_X10_30S = 1260,
  CONFIG_WHEEL_VOLTAGE_X10_32S = 1344,
  CONFIG_WHEEL_VOLTAGE_X10_36S = 1512,
  CONFIG_WHEEL_VOLTAGE_X10_40S = 1680,
  CONFIG_WHEEL_VOLTAGE_X10_42S = 1764,
  CONFIG_TIMEZONE_UTC_M12 = 0,
  CONFIG_TIMEZONE_UTC_M11 = 4,
  CONFIG_TIMEZONE_UTC_M10 = 8,
  CONFIG_TIMEZONE_UTC_M9 = 12,
  CONFIG_TIMEZONE_UTC_M8 = 16,
  CONFIG_TIMEZONE_UTC_M7 = 20,
  CONFIG_TIMEZONE_UTC_M6 = 24,
  CONFIG_TIMEZONE_UTC_M5 = 28,
  CONFIG_TIMEZONE_UTC_M4 = 32,
  CONFIG_TIMEZONE_UTC_M3 = 36,
  CONFIG_TIMEZONE_UTC_M2 = 40,
  CONFIG_TIMEZONE_UTC_M1 = 44,
  CONFIG_TIMEZONE_UTC_0 = 48,
  CONFIG_TIMEZONE_UTC_P1 = 52,
  CONFIG_TIMEZONE_UTC_P2 = 56,
  CONFIG_TIMEZONE_UTC_P3 = 60,
  CONFIG_TIMEZONE_UTC_P4 = 64,
  CONFIG_TIMEZONE_UTC_P5 = 68,
  CONFIG_TIMEZONE_UTC_P6 = 72,
  CONFIG_TIMEZONE_UTC_P7 = 76,
  CONFIG_TIMEZONE_UTC_P8 = 80,
  CONFIG_TIMEZONE_UTC_P9 = 84,
  CONFIG_TIMEZONE_UTC_P10 = 88,
  CONFIG_TIMEZONE_UTC_P11 = 92,
  CONFIG_TIMEZONE_UTC_P12 = 96,
  CONFIG_TIMEZONE_UTC_P13 = 100,
  CONFIG_TIMEZONE_UTC_P14 = 104,
  CONFIG_PWM_ALARM_SOURCE_APP = 0,
  CONFIG_PWM_ALARM_SOURCE_FIXED = 1,
};

// Значения вне допустимых заменяются значениями по умолчанию
static inline void configSanitize(fw_config_t& c)
{
  switch(c.screen1_type) { case 0: case 1: case 2: case 3: case 4: break; default: c.screen1_type = 1; }
  switch(c.screen1_v1) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen1_v1 = 6; }
  switch(c.screen1_v2) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen1_v2 = 0; }
  switch(c.screen1_v3) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen1_v3 = 1; }
  switch(c.screen1_v4) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen1_v4 = 5; }
  switch(c.screen2_type) { case 0: case 1: case 2: case 3: case 4: break; default: c.screen2_type = 2; }
  switch(c.screen2_v1) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen2_v1 = 2; }
  switch(c.screen2_v2) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen2_v2 = 3; }
  switch(c.screen2_v3) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen2_v3 = 4; }
  switch(c.screen2_v4) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen2_v4 = 5; }
  switch(c.screen3_type) { case 0: case 1: case 2: case 3: case 4: break; default: c.screen3_type = 0; }
  switch(c.screen3_v1) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen3_v1 = 2; }
  switch(c.screen3_v2) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen3_v2 = 3; }
  switch(c.screen3_v3) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen3_v3 = 4; }
  switch(c.screen3_v4) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen3_v4 = 5; }
  switch(c.screen4_type) { case 0: case 1: case 2: case 3: case 4: break; default: c.screen4_type = 0; }
  switch(c.screen4_v1) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen4_v1 = 2; }
  switch(c.screen4_v2) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen4_v2 = 3; }
  switch(c.screen4_v3) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen4_v3 = 4; }
  switch(c.screen4_v4) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen4_v4 = 5; }
  switch(c.screen5_type) { case 0: case 1: case 2: case 3: case 4: break; default: c.screen5_type = 0; }
  switch(c.screen5_v1) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen5_v1 = 2; }
  switch(c.screen5_v2) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen5_v2 = 3; }
  switch(c.screen5_v3) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen5_v3 = 4; }
  switch(c.screen5_v4) { case 0: case 1: case 2: case 3: case 4: case 5: case 6: break; default: c.screen5_v4 = 5; }
  switch(c.wheel_voltage_x10) { case 0: case 840: case 1008: case 1260: case 1344: case 1512: case 1680: case 1764: break; default: c.wheel_voltage_x10 = 0; }
  switch(c.timezone) { case 0: case 4: case 8: case 12: case 16: case 20: case 24: case 28: case 32: case 36: case 40: case 44: case 48: case 52: case 56: case 60: case 64: case 68: case 72: case 76: case 80: case 84: case 88: case 92: case 96: case 100: case 104: break; default: c.timezone = 60; }
  if(c.charge_screen > 1) c.charge_screen = 1;
  if(c.pwm_alarm > 1) c.pwm_alarm = 1;
  switch(c.pwm_alarm_source) { case 0: case 1: break; default: c.pwm_alarm_source = 0; }
  if(c.pwm_alarm_threshold < 30 || c.pwm_alarm_threshold > 99) c.pwm_alarm_threshold = 80;
  if(c.brightness < 5 || c.brightness > 100) c.brightness = 100;
  if(c.intro_logo > 1) c.intro_logo = 1;
  if(c.intro_clawd > 1) c.intro_clawd = 1;
  if(c.intro_wave > 1) c.intro_wave = 1;
  if(c.intro_break > 1) c.intro_break = 1;
  if(c.intro_explosion > 1) c.intro_explosion = 1;
  if(c.poweroff_animation > 1) c.poweroff_animation = 1;
  if(c.power_on_ms < 100 || c.power_on_ms > 3000) c.power_on_ms = 500;
  if(c.power_off_ms < 500 || c.power_off_ms > 5000) c.power_off_ms = 1000;
  if(c.ble_update > 1) c.ble_update = 1;
}

// Описание настроек для веб-конфигуратора: config.json с адресами полей
#define FW_CONFIG_SCHEMA \
  "{\"version\":2,\"groups\":[{\"title\":\"Экраны\",\"help\":\"Страницы, которые переключаются коротким нажати" \
  "ем кнопки: до пяти, в любом порядке\",\"widget\":\"screens\",\"fields\":[{\"key\":\"screen1_type\",\"type\":\"" \
  "u8\",\"default\":1,\"label\":\"Экран 1\",\"help\":\"Экраны идут по порядку, пустые пропускаются. Поля: у э" \
  "крана с двумя значениями 1 - верхняя строка (заряд или напряжение колеса либо часы), 2 и 3 - кру" \
  "пные значения слева и справа; у сетки 1-4 - клетки по строкам\",\"options\":[{\"value\":0,\"id\":\"NONE\"" \
  ",\"label\":\"Нет\"},{\"value\":1,\"id\":\"MAIN\",\"label\":\"Два значения\"},{\"value\":2,\"id\":\"GRID\",\"label\":\"Ч" \
  "етыре значения\"},{\"value\":3,\"id\":\"INFO\",\"label\":\"Связь и версии\"},{\"value\":4,\"id\":\"CLOCK\",\"label" \
  "\":\"Часы\"}],\"offset\":0,\"size\":1},{\"key\":\"screen1_v1\",\"type\":\"u8\",\"default\":6,\"label\":\"Экран 1, по" \
  "ле 1\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\"" \
  ":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\"" \
  ":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"" \
  "value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":1,\"" \
  "size\":1},{\"key\":\"screen1_v2\",\"type\":\"u8\",\"default\":0,\"label\":\"Экран 1, поле 2\",\"options\":[{\"valu" \
  "e\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"" \
  "id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"" \
  "},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"l" \
  "abel\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":2,\"size\":1},{\"key\":\"screen" \
  "1_v3\",\"type\":\"u8\",\"default\":1,\"label\":\"Экран 1, поле 3\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"labe" \
  "l\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"З" \
  "аряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMP" \
  "ERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"v" \
  "alue\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":3,\"size\":1},{\"key\":\"screen1_v4\",\"type\":\"u8\",\"defa" \
  "ult\":5,\"label\":\"Экран 1, поле 4\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"" \
  "value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"valu" \
  "e\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Темпе" \
  "ратура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"l" \
  "abel\":\"Часы\"}],\"offset\":4,\"size\":1},{\"key\":\"screen2_type\",\"type\":\"u8\",\"default\":2,\"label\":\"Экран" \
  " 2\",\"options\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"value\":1,\"id\":\"MAIN\",\"label\":\"Два значени" \
  "я\"},{\"value\":2,\"id\":\"GRID\",\"label\":\"Четыре значения\"},{\"value\":3,\"id\":\"INFO\",\"label\":\"Связь и ве" \
  "рсии\"},{\"value\":4,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":5,\"size\":1},{\"key\":\"screen2_v1\",\"type\":" \
  "\"u8\",\"default\":2,\"label\":\"Экран 2, поле 1\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость," \
  " км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, " \
  "%\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"lab" \
  "el\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":" \
  "\"CLOCK\",\"label\":\"Часы\"}],\"offset\":6,\"size\":1},{\"key\":\"screen2_v2\",\"type\":\"u8\",\"default\":3,\"label" \
  "\":\"Экран 2, поле 2\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\"" \
  ":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VO" \
  "LTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контро" \
  "ллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}" \
  "],\"offset\":7,\"size\":1},{\"key\":\"screen2_v3\",\"type\":\"u8\",\"default\":4,\"label\":\"Экран 2, поле 3\",\"op" \
  "tions\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"" \
  "},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряже" \
  "ние колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5," \
  "\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":8,\"size\":1}," \
  "{\"key\":\"screen2_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Экран 2, поле 4\",\"options\":[{\"value\":0,\"id\"" \
  ":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHAR" \
  "GE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value" \
  "\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пр" \
  "обег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":9,\"size\":1},{\"key\":\"screen3_type\",\"" \
  "type\":\"u8\",\"default\":0,\"label\":\"Экран 3\",\"options\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"valu" \
  "e\":1,\"id\":\"MAIN\",\"label\":\"Два значения\"},{\"value\":2,\"id\":\"GRID\",\"label\":\"Четыре значения\"},{\"val" \
  "ue\":3,\"id\":\"INFO\",\"label\":\"Связь и версии\"},{\"value\":4,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":10" \
  ",\"size\":1},{\"key\":\"screen3_v1\",\"type\":\"u8\",\"default\":2,\"label\":\"Экран 3, поле 1\",\"options\":[{\"va" \
  "lue\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2" \
  ",\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, " \
  "В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\"," \
  "\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":11,\"size\":1},{\"key\":\"scr" \
  "een3_v2\",\"type\":\"u8\",\"default\":3,\"label\":\"Экран 3, поле 2\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"l" \
  "abel\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\"" \
  ":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"T" \
  "EMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"}," \
  "{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":12,\"size\":1},{\"key\":\"screen3_v3\",\"type\":\"u8\",\"" \
  "default\":4,\"label\":\"Экран 3, поле 3\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"" \
  "},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"" \
  "value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Т" \
  "емпература контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK" \
  "\",\"label\":\"Часы\"}],\"offset\":13,\"size\":1},{\"key\":\"screen3_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Эк" \
  "ран 3, поле 4\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM" \
  "\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE" \
  "\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера" \
  ", °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"of" \
  "fset\":14,\"size\":1},{\"key\":\"screen4_type\",\"type\":\"u8\",\"default\":0,\"label\":\"Экран 4\",\"options\":[{\"" \
  "value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"value\":1,\"id\":\"MAIN\",\"label\":\"Два значения\"},{\"value\":2,\"i" \
  "d\":\"GRID\",\"label\":\"Четыре значения\"},{\"value\":3,\"id\":\"INFO\",\"label\":\"Связь и версии\"},{\"value\":4" \
  ",\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":15,\"size\":1},{\"key\":\"screen4_v1\",\"type\":\"u8\",\"default\":2" \
  ",\"label\":\"Экран 4, поле 1\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\"" \
  ":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"" \
  "id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура" \
  " контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":" \
  "\"Часы\"}],\"offset\":16,\"size\":1},{\"key\":\"screen4_v2\",\"type\":\"u8\",\"default\":3,\"label\":\"Экран 4, пол" \
  "е 2\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":" \
  "\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":" \
  "\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"v" \
  "alue\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":17,\"" \
  "size\":1},{\"key\":\"screen4_v3\",\"type\":\"u8\",\"default\":4,\"label\":\"Экран 4, поле 3\",\"options\":[{\"valu" \
  "e\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"" \
  "id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"" \
  "},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"l" \
  "abel\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":18,\"size\":1},{\"key\":\"scree" \
  "n4_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Экран 4, поле 4\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"lab" \
  "el\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"" \
  "Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEM" \
  "PERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"" \
  "value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":19,\"size\":1},{\"key\":\"screen5_type\",\"type\":\"u8\",\"" \
  "default\":0,\"label\":\"Экран 5\",\"options\":[{\"value\":0,\"id\":\"NONE\",\"label\":\"Нет\"},{\"value\":1,\"id\":\"M" \
  "AIN\",\"label\":\"Два значения\"},{\"value\":2,\"id\":\"GRID\",\"label\":\"Четыре значения\"},{\"value\":3,\"id\":\"" \
  "INFO\",\"label\":\"Связь и версии\"},{\"value\":4,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":20,\"size\":1},{" \
  "\"key\":\"screen5_v1\",\"type\":\"u8\",\"default\":2,\"label\":\"Экран 5, поле 1\",\"options\":[{\"value\":0,\"id\":" \
  "\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARG" \
  "E\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\"" \
  ":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Про" \
  "бег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":21,\"size\":1},{\"key\":\"screen5_v2\",\"ty" \
  "pe\":\"u8\",\"default\":3,\"label\":\"Экран 5, поле 2\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скоро" \
  "сть, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд коле" \
  "са, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\"," \
  "\"label\":\"Температура контроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"" \
  "id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":22,\"size\":1},{\"key\":\"screen5_v3\",\"type\":\"u8\",\"default\":4,\"" \
  "label\":\"Экран 5, поле 3\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1" \
  ",\"id\":\"PWM\",\"label\":\"ШИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id" \
  "\":\"VOLTAGE\",\"label\":\"Напряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура к" \
  "онтроллера, °C\"},{\"value\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Ч" \
  "асы\"}],\"offset\":23,\"size\":1},{\"key\":\"screen5_v4\",\"type\":\"u8\",\"default\":5,\"label\":\"Экран 5, поле " \
  "4\",\"options\":[{\"value\":0,\"id\":\"SPEED\",\"label\":\"Скорость, км/ч\"},{\"value\":1,\"id\":\"PWM\",\"label\":\"Ш" \
  "ИМ, %\"},{\"value\":2,\"id\":\"CHARGE\",\"label\":\"Заряд колеса, %\"},{\"value\":3,\"id\":\"VOLTAGE\",\"label\":\"Н" \
  "апряжение колеса, В\"},{\"value\":4,\"id\":\"TEMPERATURE\",\"label\":\"Температура контроллера, °C\"},{\"val" \
  "ue\":5,\"id\":\"TRIP\",\"label\":\"Пробег, км\"},{\"value\":6,\"id\":\"CLOCK\",\"label\":\"Часы\"}],\"offset\":24,\"si" \
  "ze\":1},{\"key\":\"wheel_voltage_x10\",\"type\":\"u16\",\"default\":0,\"label\":\"Батарея колеса\",\"help\":\"Дисп" \
  "лей узнаёт её сам по напряжению и заряду колеса, но её можно задать вручную\",\"options\":[{\"value\"" \
  ":0,\"id\":\"AUTO\",\"label\":\"Автоматически\"},{\"value\":840,\"id\":\"20S\",\"label\":\"84 В (20S)\"},{\"value\":1" \
  "008,\"id\":\"24S\",\"label\":\"100,8 В (24S)\"},{\"value\":1260,\"id\":\"30S\",\"label\":\"126 В (30S)\"},{\"value\"" \
  ":1344,\"id\":\"32S\",\"label\":\"134,4 В (32S)\"},{\"value\":1512,\"id\":\"36S\",\"label\":\"151,2 В (36S)\"},{\"va" \
  "lue\":1680,\"id\":\"40S\",\"label\":\"168 В (40S)\"},{\"value\":1764,\"id\":\"42S\",\"label\":\"176,4 В (42S)\"}],\"" \
  "offset\":25,\"size\":2},{\"key\":\"timezone\",\"type\":\"u8\",\"default\":60,\"label\":\"Часовой пояс\",\"options\"" \
  ":[{\"value\":0,\"id\":\"UTC_M12\",\"label\":\"UTC-12\"},{\"value\":4,\"id\":\"UTC_M11\",\"label\":\"UTC-11\"},{\"valu" \
  "e\":8,\"id\":\"UTC_M10\",\"label\":\"UTC-10\"},{\"value\":12,\"id\":\"UTC_M9\",\"label\":\"UTC-9\"},{\"value\":16,\"id" \
  "\":\"UTC_M8\",\"label\":\"UTC-8\"},{\"value\":20,\"id\":\"UTC_M7\",\"label\":\"UTC-7\"},{\"value\":24,\"id\":\"UTC_M6\"" \
  ",\"label\":\"UTC-6\"},{\"value\":28,\"id\":\"UTC_M5\",\"label\":\"UTC-5\"},{\"value\":32,\"id\":\"UTC_M4\",\"label\":\"" \
  "UTC-4\"},{\"value\":36,\"id\":\"UTC_M3\",\"label\":\"UTC-3\"},{\"value\":40,\"id\":\"UTC_M2\",\"label\":\"UTC-2\"},{\"" \
  "value\":44,\"id\":\"UTC_M1\",\"label\":\"UTC-1\"},{\"value\":48,\"id\":\"UTC_0\",\"label\":\"UTC\"},{\"value\":52,\"id" \
  "\":\"UTC_P1\",\"label\":\"UTC+1\"},{\"value\":56,\"id\":\"UTC_P2\",\"label\":\"UTC+2, Калининград\"},{\"value\":60," \
  "\"id\":\"UTC_P3\",\"label\":\"UTC+3, Москва, Минск, Стамбул\"},{\"value\":64,\"id\":\"UTC_P4\",\"label\":\"UTC+4," \
  " Самара, Тбилиси, Ереван, Баку\"},{\"value\":68,\"id\":\"UTC_P5\",\"label\":\"UTC+5, Екатеринбург, Ташкент" \
  ", Астана\"},{\"value\":72,\"id\":\"UTC_P6\",\"label\":\"UTC+6, Омск, Бишкек\"},{\"value\":76,\"id\":\"UTC_P7\",\"l" \
  "abel\":\"UTC+7, Новосибирск, Красноярск\"},{\"value\":80,\"id\":\"UTC_P8\",\"label\":\"UTC+8, Иркутск, Пекин" \
  "\"},{\"value\":84,\"id\":\"UTC_P9\",\"label\":\"UTC+9, Якутск, Токио, Сеул\"},{\"value\":88,\"id\":\"UTC_P10\",\"l" \
  "abel\":\"UTC+10, Владивосток\"},{\"value\":92,\"id\":\"UTC_P11\",\"label\":\"UTC+11, Магадан, Сахалин\"},{\"va" \
  "lue\":96,\"id\":\"UTC_P12\",\"label\":\"UTC+12, Камчатка\"},{\"value\":100,\"id\":\"UTC_P13\",\"label\":\"UTC+13\"}" \
  ",{\"value\":104,\"id\":\"UTC_P14\",\"label\":\"UTC+14\"}],\"offset\":27,\"size\":1},{\"key\":\"charge_screen\",\"ty" \
  "pe\":\"bool\",\"default\":true,\"label\":\"Экран зарядки колеса\",\"help\":\"Включается сам, пока колесо зар" \
  "яжается\",\"preview\":\"charging\",\"offset\":28,\"size\":1}]},{\"title\":\"Предупреждение о ШИМ\",\"help\":\"Ми" \
  "гание экрана, когда ШИМ подходит к пределу\",\"fields\":[{\"key\":\"pwm_alarm\",\"type\":\"bool\",\"default\"" \
  ":true,\"label\":\"Мигать экраном при превышении порога ШИМ\",\"help\":\"Чем выше ШИМ, тем чаще мигание\"" \
  ",\"offset\":29,\"size\":1},{\"key\":\"pwm_alarm_source\",\"type\":\"u8\",\"default\":0,\"label\":\"Порог\",\"option" \
  "s\":[{\"value\":0,\"id\":\"APP\",\"label\":\"Из настроек приложения LoEUC\"},{\"value\":1,\"id\":\"FIXED\",\"label" \
  "\":\"Свой\"}],\"depends\":{\"pwm_alarm\":true},\"offset\":30,\"size\":1},{\"key\":\"pwm_alarm_threshold\",\"type" \
  "\":\"u8\",\"default\":80,\"min\":30,\"max\":99,\"unit\":\"%\",\"label\":\"Свой порог ШИМ\",\"depends\":{\"pwm_alarm\"" \
  ":true,\"pwm_alarm_source\":1},\"offset\":31,\"size\":1}]},{\"title\":\"Дисплей\",\"help\":\"Яркость экрана\",\"" \
  "fields\":[{\"key\":\"brightness\",\"type\":\"u8\",\"default\":100,\"min\":5,\"max\":100,\"step\":5,\"unit\":\"%\",\"wi" \
  "dget\":\"range\",\"label\":\"Яркость\",\"offset\":32,\"size\":1}]},{\"title\":\"Анимации\",\"help\":\"Заставка при" \
  " включении и анимация выключения\",\"fields\":[{\"key\":\"intro_logo\",\"type\":\"bool\",\"default\":true,\"se" \
  "ction\":\"При включении\",\"label\":\"Логотип при включении\",\"help\":\"После логотипа играет случайная и" \
  "з отмеченных заставок. Без логотипа данные появляются сразу\",\"preview\":\"logo\",\"offset\":33,\"size\"" \
  ":1},{\"key\":\"intro_clawd\",\"type\":\"bool\",\"default\":false,\"label\":\"Заставки Clawd\",\"help\":\"Играют з" \
  "аставки Clawd\",\"widget\":\"clawd\",\"depends\":{\"intro_logo\":true},\"offset\":34,\"size\":1},{\"key\":\"intr" \
  "o_wave\",\"type\":\"bool\",\"default\":true,\"label\":\"Волна\",\"help\":\"Волна с пузырями проходит слева нап" \
  "раво и открывает данные\",\"preview\":\"wave\",\"depends\":{\"intro_logo\":true,\"intro_clawd\":false},\"sub" \
  "item\":true,\"offset\":35,\"size\":1},{\"key\":\"intro_break\",\"type\":\"bool\",\"default\":true,\"label\":\"Разр" \
  "ушение\",\"help\":\"Логотип трескается и рассыпается на куски\",\"preview\":\"break\",\"depends\":{\"intro_l" \
  "ogo\":true,\"intro_clawd\":false},\"subitem\":true,\"offset\":36,\"size\":1},{\"key\":\"intro_explosion\",\"ty" \
  "pe\":\"bool\",\"default\":true,\"label\":\"Взрыв\",\"help\":\"Логотип разлетается облаком частиц, как TNT в " \
  "Minecraft\",\"preview\":\"explosion\",\"depends\":{\"intro_logo\":true,\"intro_clawd\":false},\"subitem\":tru" \
  "e,\"offset\":37,\"size\":1},{\"key\":\"poweroff_animation\",\"type\":\"bool\",\"default\":true,\"section\":\"При " \
  "выключении\",\"label\":\"Анимация выключения\",\"help\":\"Экран сжимается в линию, затем в точку и гасне" \
  "т, как у старого телевизора\",\"preview\":\"poweroff\",\"offset\":38,\"size\":1}]},{\"title\":\"Кнопка\",\"hel" \
  "p\":\"Сколько держать кнопку, чтобы включить и выключить дисплей\",\"fields\":[{\"key\":\"power_on_ms\",\"" \
  "type\":\"u16\",\"default\":500,\"min\":100,\"max\":3000,\"step\":100,\"unit\":\"мс\",\"label\":\"Удержание для вкл" \
  "ючения\",\"offset\":39,\"size\":2},{\"key\":\"power_off_ms\",\"type\":\"u16\",\"default\":1000,\"min\":500,\"max\":" \
  "5000,\"step\":100,\"unit\":\"мс\",\"label\":\"Удержание для выключения\",\"help\":\"Более короткое нажатие пе" \
  "реключает экран\",\"offset\":41,\"size\":2}]},{\"title\":\"Обновление по Bluetooth\",\"help\":\"Функция не о" \
  "тлажена и поэтому отключена. Если интересно, пинайте разработчика @glazovy\",\"fields\":[{\"key\":\"bl" \
  "e_update\",\"type\":\"bool\",\"default\":false,\"label\":\"Разрешить обновление прошивки по Bluetooth\",\"he" \
  "lp\":\"Режим обновления включается, если при включении держать кнопку около 4 секунд. Нужен заряд " \
  "не ниже 30% или подключённый USB. Прошивать в нём дисплей сайт пока не умеет\",\"offset\":43,\"size\"" \
  ":1}]}]}"
